#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include "DXrenderer/DXpipeline.h"
#include "WindowsApp.h"

using namespace DXRplayground;

namespace
{
DXpipeline RenderPipeline;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int nCmdShow)
{
    WindowsApp::Init(hInstance, nCmdShow, L"DXR playground");
    RenderPipeline.Init(WindowsApp::GetHWND(), 1920, 1080);
    WindowsApp::Run();
    return 0;
} 

namespace DXRplayground
{
void Run()
{
    RenderPipeline.Draw();
}
}