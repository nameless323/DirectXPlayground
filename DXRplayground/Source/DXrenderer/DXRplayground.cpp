#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include "DXrenderer/RenderPipeline.h"
#include "WindowsApp.h"

#include "Scene/DummyScene.h"

using namespace DirectxPlayground;

namespace
{
RenderPipeline DirectXPipeline;
}

DummyScene scene;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int nCmdShow)
{
    WindowsApp::Init(hInstance, nCmdShow, L"DirectX Playground");
    DirectXPipeline.Init(WindowsApp::GetHWND(), 1920, 1080, &scene);
    WindowsApp::Run();
    return 0;
} 

namespace DirectxPlayground
{
void Run()
{
    DirectXPipeline.Render(&scene);
}

void Shutdown()
{
    DirectXPipeline.Shutdown();
}
}