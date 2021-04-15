#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <thread>

#include "DXrenderer/RenderPipeline.h"
#include "WindowsApp.h"
#include "Utils/FileWatcher.h"

#include "Scene/GltfViewer.h"

using namespace DirectxPlayground;

namespace
{
RenderPipeline DirectXPipeline;
FileWatcher* ShaderWatcher = nullptr;
}

GltfViewer scene;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int nCmdShow)
{
    WindowsApp::Init(hInstance, nCmdShow, L"DirectX Playground");
    DirectXPipeline.Init(WindowsApp::GetHWND(), 1920, 1080, &scene);
    WindowsApp::Run();
    return 0;
} 

namespace DirectxPlayground
{
void Init()
{
    ShaderWatcher = new FileWatcher(L"C:\\Repos\\DXRplayground\\DXRplayground\\tmp"); // Will be deleted in DirectoryModificationCallback in FileWatcher. In if (errorCode == ERROR_OPERATION_ABORTED)
    std::thread shaderWatcherThread(std::ref(*ShaderWatcher));
    shaderWatcherThread.detach();
}

void Run()
{
    DirectXPipeline.Render(&scene);
}

void Shutdown()
{
    ShaderWatcher->Shutdown();
    DirectXPipeline.Shutdown();
}
}