#pragma once

// Check https://qualapps.blogspot.com/2010/05/understanding-readdirectorychangesw.html to better understand this abomination.

#include <atomic>
#include <cassert>
#include <string>
#include <iostream>
#include <thread>
#include <atlstr.h>
#include <shlwapi.h>

class FileWatcher
{
public:
    std::atomic<bool> m_isRunning{ false };
    std::string m_path;
    HANDLE m_dirHandle;
    OVERLAPPED m_overlapped;
    static constexpr DWORD m_bufferSize = 16384;
    std::vector<BYTE> m_buffer;
    std::vector<BYTE> m_backupBuffer;
    CStringW m_wstrDirectory = L"C:\\Repos\\DXRplayground\\DXRplayground\\tmp";

    FileWatcher(std::string path)
        : m_path(std::move(path))
    {
        ZeroMemory(&m_overlapped, sizeof(OVERLAPPED));
        m_overlapped.hEvent = this;

        m_buffer.resize(m_bufferSize);
        m_backupBuffer.resize(m_bufferSize);

        std::wstring pth = L"C:\\Repos\\DXRplayground\\DXRplayground\\tmp";
        m_dirHandle = CreateFile(
            pth.c_str(),
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

    void BackupBuffer(DWORD size)
    {
        memcpy(&m_backupBuffer[0], &m_buffer[0], size);
    }

    void ReadDirectoryChanges()
    {
        DWORD dwBytes = 0;
        BOOL sucess = ReadDirectoryChangesW(m_dirHandle, &m_buffer[0], DWORD(m_buffer.size()), true, FILE_NOTIFY_CHANGE_LAST_WRITE, &dwBytes, &m_overlapped, &DirectoryModificationCallback);
    }

    void ProcessDirectoryNotification()
    {
        BYTE* data = m_backupBuffer.data();
        while (true) // Do some check with counter
        {
            FILE_NOTIFY_INFORMATION& fni = (FILE_NOTIFY_INFORMATION&)*data;

            CStringW filename(fni.FileName, fni.FileNameLength / sizeof(wchar_t));
            if (m_wstrDirectory.Right(1) != L"\\")
                filename = m_wstrDirectory + L"\\" + filename;
            else
                filename = m_wstrDirectory + filename;

            LPCWSTR wszFilename = PathFindFileNameW(filename);
            int len = lstrlenW(wszFilename);

            if (len <= 12 && wcschr(wszFilename, L'~'))
            {
                wchar_t wbuf[MAX_PATH];
                if (GetLongPathNameW(filename, wbuf, _countof(wbuf)) > 0)
                    filename = wbuf;
            }
            if (!fni.NextEntryOffset)
                break;
            data += fni.NextEntryOffset;
        }
    }

    static VOID CALLBACK DirectoryModificationCallback(DWORD errorCode, DWORD numberOfBytesTransfered, LPOVERLAPPED overlapped)
    {
        FileWatcher* watcher = (FileWatcher*)overlapped->hEvent;

        if (errorCode == ERROR_OPERATION_ABORTED)
            return;

        if (numberOfBytesTransfered == 0)
            return;

        watcher->BackupBuffer(numberOfBytesTransfered); // To avoid race condition.

        watcher->ReadDirectoryChanges();
        watcher->ProcessDirectoryNotification();
    }

    void operator()()
    {
        assert(m_isRunning && "File watcher thread is already running");
        m_isRunning = true;

        ReadDirectoryChanges();

        /*
        while (m_isRunning)
        {
            OutputDebugStringA("In the thread\n");
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        OutputDebugStringA("Thread stopped\n");*/
    }

    void Shutdown()
    {
        m_isRunning = false;
        CancelIo(m_dirHandle);
        CloseHandle(m_dirHandle);
    }
};
