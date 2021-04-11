#pragma once

#include <atomic>
#include <string>
#include <iostream>
#include <thread>

class FileWatcher
{
public:
    std::atomic<bool> stopThread{ false };

    FileWatcher(std::string path)
    {}

    void operator()()
    {
        while (!stopThread)
        {
            OutputDebugStringA("In the thread\n");
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        OutputDebugStringA("Thread stopped\n");
    }
};
