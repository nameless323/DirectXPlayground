#pragma once

// Check https://qualapps.blogspot.com/2010/05/understanding-readdirectorychangesw.html to better understand this abomination.

// IMPORTANT: Instances of FileWatcher should be created on heap only. They are deleted in DirectoryModificationCallback when no more i/o events are pending (so, do not delete manually).

#include <vector>
#include <string>
#include <windows.h>

#include "Utils/ThreadSafeQueue.h"

namespace DirectxPlayground
{

class FileWatcher
{
public:
    FileWatcher(std::wstring path, UINT watchFlags = FILE_NOTIFY_CHANGE_LAST_WRITE);

    void Shutdown();
    void operator()();

    ThreadSafeQueue<std::wstring>& GetModifiedFilesQueue();

private:
    static VOID CALLBACK DirectoryModificationCallback(DWORD errorCode, DWORD numberOfBytesTransfered, LPOVERLAPPED overlapped);

    void ProcessDirectoryNotification();
    void SleepAlertable();
    void BackupBuffer(DWORD size);
    void ReadDirectoryChanges();

    static constexpr DWORD m_bufferSize = 16384;

    UINT m_watchFlags;
    HANDLE m_dirHandle;
    OVERLAPPED m_overlapped;
    std::wstring m_path;
    std::vector<BYTE> m_buffer;
    std::vector<BYTE> m_backupBuffer;
    ThreadSafeQueue<std::wstring> m_modifiedFilesQueue;
};

inline ThreadSafeQueue<std::wstring>& FileWatcher::GetModifiedFilesQueue()
{
    return m_modifiedFilesQueue;
}

inline void FileWatcher::SleepAlertable()
{
    SleepEx(INFINITE, true);
}

inline void FileWatcher::ReadDirectoryChanges()
{
    DWORD dwBytes = 0;
    BOOL sucess = ReadDirectoryChangesW(m_dirHandle, &m_buffer[0], DWORD(m_buffer.size()), true, m_watchFlags, &dwBytes, &m_overlapped, &DirectoryModificationCallback);
}

inline void FileWatcher::BackupBuffer(DWORD size)
{
    memcpy(&m_backupBuffer[0], &m_buffer[0], size);
}

inline void FileWatcher::operator()()
{
    ReadDirectoryChanges();
    SleepAlertable();
}
}
