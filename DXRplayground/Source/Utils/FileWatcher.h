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
    FileWatcher(std::wstring path, UINT watchFlags = FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_FILE_NAME);

    void Shutdown();
    void operator()();

    ThreadSafeQueue<std::wstring>& GetModifiedFilesQueue();

private:
    static VOID CALLBACK DirectoryModificationCallback(DWORD errorCode, DWORD numberOfBytesTransfered, LPOVERLAPPED overlapped);

    void ProcessDirectoryNotification();
    void SleepAlertable();
    void BackupBuffer(DWORD size);
    void ReadDirectoryChanges();

    static constexpr DWORD BufferSize = 16384;

    UINT mWatchFlags;
    HANDLE mDirHandle;
    OVERLAPPED mOverlapped;
    std::wstring mPath;
    std::vector<BYTE> mBuffer;
    std::vector<BYTE> mBackupBuffer;
    ThreadSafeQueue<std::wstring> mModifiedFilesQueue;
};

inline ThreadSafeQueue<std::wstring>& FileWatcher::GetModifiedFilesQueue()
{
    return mModifiedFilesQueue;
}

inline void FileWatcher::SleepAlertable()
{
    SleepEx(INFINITE, true);
}

inline void FileWatcher::ReadDirectoryChanges()
{
    DWORD dwBytes = 0;
    BOOL sucess = ReadDirectoryChangesW(mDirHandle, &mBuffer[0], DWORD(mBuffer.size()), true, mWatchFlags, &dwBytes, &mOverlapped, &DirectoryModificationCallback);
}

inline void FileWatcher::BackupBuffer(DWORD size)
{
    memcpy(&mBackupBuffer[0], &mBuffer[0], size);
}

inline void FileWatcher::operator()()
{
    ReadDirectoryChanges();
    SleepAlertable();
}
}
