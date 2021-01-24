#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include "WindowsApp.h"

using namespace DXRplayground;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int nCmdShow)
{
    WindowsApp::Init(hInstance, nCmdShow, L"DXR playground");
    WindowsApp::Run();
    return 0;
} 

namespace DXRplayground
{
void Run()
{
}
}