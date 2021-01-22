#pragma once

#include <string>
#include <windows.h>

namespace DXRplayground::WindowsApp
{
bool Init(HINSTANCE hInstance, int nCmdShow, std::wstring caption);
long int Run();
void Shutdown();
void ChangeFullscreenMode(bool fullScreen);

HWND GetHWND();
}
