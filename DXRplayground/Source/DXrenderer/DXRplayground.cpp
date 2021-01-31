#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include "DXrenderer/RenderPipeline.h"
#include "WindowsApp.h"

using namespace DirectxPlayground;

namespace
{
RenderPipeline DirectXPipeline;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int nCmdShow)
{
    WindowsApp::Init(hInstance, nCmdShow, L"DirectX Playground");
    DirectXPipeline.Init(WindowsApp::GetHWND(), 1920, 1080);
    WindowsApp::Run();
    return 0;
} 

namespace DirectxPlayground
{
void Run()
{
    DirectXPipeline.Render();
}
}