#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <thread>

#include "DXrenderer/RenderPipeline.h"
#include "WindowsApp.h"

#include "Scene/GltfViewer.h"
#include "Scene/PbrTester.h"
#include "Scene/RtTester.h"

using namespace DirectxPlayground;

namespace
{
RenderPipeline DirectXPipeline;
}

RtTester scene;

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE prevInstance, _In_ PSTR cmdLine, _In_ int nCmdShow)
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
}

void Run()
{
    DirectXPipeline.Render(&scene);
}

void Shutdown()
{
    DirectXPipeline.Shutdown();
}
}