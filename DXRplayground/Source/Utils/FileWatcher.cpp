#include "Utils/FileWatcher.h"

#include <cassert>
#include <atlstr.h>
#include <shlwapi.h>

namespace DirectxPlayground
{
namespace
{
static constexpr UINT CounterMaxValue = 10000;
}

FileWatcher::FileWatcher(std::wstring path, UINT watchFlags /*= FILE_NOTIFY_CHANGE_LAST_WRITE*/)
    : m_path(std::move(path))
    , m_watchFlags(watchFlags)
{

    ZeroMemory(&m_overlapped, sizeof(OVERLAPPED));
    m_overlapped.hEvent = this;

    m_buffer.resize(m_bufferSize);
    m_backupBuffer.resize(m_bufferSize);

    m_dirHandle = CreateFile(
        m_path.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL
    );
    if (m_dirHandle == INVALID_HANDLE_VALUE)
        assert(false);
}

void FileWatcher::Shutdown()
{
    if (m_dirHandle == INVALID_HANDLE_VALUE)
    {
        assert(false);
        return;
    }
    CancelIo(m_dirHandle);
    CloseHandle(m_dirHandle);
    m_dirHandle = INVALID_HANDLE_VALUE;
}

VOID FileWatcher::DirectoryModificationCallback(DWORD errorCode, DWORD numberOfBytesTransfered, LPOVERLAPPED overlapped)
{
    FileWatcher* watcher = (FileWatcher*)overlapped->hEvent;

    if (errorCode == ERROR_OPERATION_ABORTED) // Reason of this error code - CancelIo was called. Watcher may lay in peace now.
    {
        delete watcher;
        return;
    }

    if (numberOfBytesTransfered == 0)
    {
        delete watcher; // Not sure here. In theory CancelIo should trigger errorCode == ERROR_OPERATION_ABORTED, in practice, it says "no errors" but 0 bytes are transferred.
        return;
    }

    watcher->BackupBuffer(numberOfBytesTransfered); // To avoid race condition.

    watcher->ReadDirectoryChanges();
    watcher->ProcessDirectoryNotification();
    watcher->SleepAlertable();
}

void FileWatcher::ProcessDirectoryNotification()
{
    BYTE* data = m_backupBuffer.data();
    UINT i = 0;
    while (i < CounterMaxValue) // Do some check with counter
    {
        FILE_NOTIFY_INFORMATION& fni = (FILE_NOTIFY_INFORMATION&)*data;

        std::wstring filename(fni.FileName, fni.FileNameLength / sizeof(wchar_t));
        if (m_path.back() != L'\\')
            filename = m_path + L"\\" + filename;
        else
            filename = m_path + filename;

        LPCWSTR wszFilename = PathFindFileNameW(filename.c_str());
        int len = lstrlenW(wszFilename);

        if (len <= 12 && wcschr(wszFilename, L'~'))
        {
            wchar_t wbuf[MAX_PATH];
            if (GetLongPathNameW(filename.c_str(), wbuf, _countof(wbuf)) > 0)
                filename = wbuf;
        }
        m_modifiedFilesQueue.Push(std::move(filename));
        if (!fni.NextEntryOffset)
            break;
        data += fni.NextEntryOffset;

        ++i;
    }
    assert(i != CounterMaxValue && "Infinity loop in FileWatcher::ProcessDirectoryNotification");
}
}