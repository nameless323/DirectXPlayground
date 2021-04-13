#pragma once

#include <atomic>
#include <cassert>
#include <string>
#include <iostream>
#include <thread>

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

    void ReadDirectoryChanges()
    {
        DWORD dwBytes = 0;
        BOOL sucess = ReadDirectoryChangesW(m_dirHandle, &m_buffer[0], m_buffer.size(), true, FILE_NOTIFY_CHANGE_LAST_WRITE, &dwBytes, &m_overlapped, &DirectoryModificationCallback);
    }

    static VOID CALLBACK DirectoryModificationCallback(DWORD errorCode, DWORD numberOfBytesTransfered, LPOVERLAPPED overlapped)
    {
        FileWatcher* watcher = (FileWatcher*)overlapped->hEvent;
    }

    void operator()()
    {
        assert(m_isRunning && "File watcher thread is already running");
        m_isRunning = true;




        while (m_isRunning)
        {
            OutputDebugStringA("In the thread\n");
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        OutputDebugStringA("Thread stopped\n");
    }

    void Shutdown()
    {
        m_isRunning = false;
        CancelIo(m_dirHandle);
        CloseHandle(m_dirHandle);
    }
};
