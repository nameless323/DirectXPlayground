#include "WindowsApp.h"

#include "External/IMGUI/imgui_impl_win32.h"

namespace DirectxPlayground
{
void Init();
void Run();
void Shutdown();
}
namespace ImGui
{
IMGUI_IMPL_API LRESULT ImplWinWndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
}

namespace DirectxPlayground::WindowsApp
{
namespace
{
const UINT WindowStyle = WS_OVERLAPPEDWINDOW;
RECT WindowRect = {};
std::wstring WindowCaption;
bool IsFullscreen = false;
HWND Hwnd;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

bool Init(HINSTANCE hInstance, int nCmdShow, std::wstring caption)
{
    IsFullscreen = false;
    WindowCaption = caption;

    WNDCLASSEX windowClass = {};
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.lpszClassName = caption.c_str();
    RegisterClassEx(&windowClass);

    RECT windowRect = { 0, 0, 1920, 1080 };
    if (!AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE))
        return false;

    Hwnd = CreateWindow(windowClass.lpszClassName,
        WindowCaption.c_str(),
        WindowStyle,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    if (Hwnd == nullptr)
    {
        return false;
    }

    ShowWindow(Hwnd, nCmdShow);

    DirectxPlayground::Init();

    return true;
}

long int Run()
{
    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            DirectxPlayground::Run();
        }
    }
    Shutdown();

    return static_cast<long int>(msg.wParam);
}

void Shutdown()
{
    DirectxPlayground::Shutdown();
}

void ChangeFullscreenMode(bool fullScreen)
{
    if (fullScreen == IsFullscreen)
        return;

    IsFullscreen = fullScreen;
    if (!IsFullscreen)
    {
        SetWindowLong(Hwnd, GWL_STYLE, WindowStyle);

        SetWindowPos(
            Hwnd,
            HWND_NOTOPMOST,
            WindowRect.left,
            WindowRect.top,
            WindowRect.right - WindowRect.left,
            WindowRect.bottom - WindowRect.top,
            SWP_FRAMECHANGED | SWP_NOACTIVATE);

        ShowWindow(Hwnd, SW_NORMAL);
        return;
    }
    GetWindowRect(Hwnd, &WindowRect);

    UINT fullScreenWindowStyle = WindowStyle & ~(WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU | WS_THICKFRAME);
    SetWindowLong(Hwnd, GWL_STYLE, fullScreenWindowStyle);

    DEVMODE devMode = {};
    devMode.dmSize = sizeof(DEVMODE);
    EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &devMode);

    SetWindowPos(
        Hwnd,
        HWND_TOPMOST,
        devMode.dmPosition.x,
        devMode.dmPosition.y,
        devMode.dmPosition.x + devMode.dmPelsWidth,
        devMode.dmPosition.y + devMode.dmPelsHeight,
        SWP_FRAMECHANGED | SWP_NOACTIVATE);

    ShowWindow(Hwnd, SW_MAXIMIZE);
}

LRESULT WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    ImGui::ImplWinWndProcHandler(hwnd, message, wParam, lParam);

    switch (message)
    {
    case WM_CREATE:
    {
        LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));

        // [a_vorontcov] For now only keyboard, joystick and mouse.
        RAWINPUTDEVICE rid[3];

        rid[0].usUsagePage = 0x01;
        rid[0].usUsage = 0x02; // [a_vorontcov] Mouse.
        rid[0].dwFlags = 0;
        rid[0].hwndTarget = hwnd;

        rid[1].usUsagePage = 0x01;
        rid[1].usUsage = 0x06; // [a_vorontcov] Keyboard.
        rid[1].dwFlags = 0;
        rid[1].hwndTarget = hwnd;

        rid[2].usUsagePage = 0x01;
        rid[2].usUsage = 0x04; // [a_vorontcov] Joystic.
        rid[2].dwFlags = 0;
        rid[2].hwndTarget = hwnd;

        //if (!RegisterRawInputDevices(rid, 3, sizeof(rid[0])))
        //    assert(false && "Couldn't register input devices");
        return 0;
    }

    case WM_INPUT:
    {
        //if (ImGui::IsAnyWindowFocused())
        //    return 0;
        // [a_vorontcov] Explanation of what's going on is here - https://docs.microsoft.com/en-us/windows/desktop/inputdev/using-raw-input

        UINT dwSize;
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
        LPBYTE lpb = new BYTE[dwSize];
        if (lpb == NULL)
            return 0;

        if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
            OutputDebugString(TEXT("GetRawInputData doesn't return correct size! \n"));
        RAWINPUT* raw = (RAWINPUT*)lpb;

        if (raw->header.dwType == RIM_TYPEKEYBOARD)
        {
            //if ((raw->data.keyboard.Flags & RI_KEY_BREAK) != 0)
                //Input::SetButtonUp(static_cast<uint32>(raw->data.keyboard.VKey));
            //else if (raw->data.keyboard.Flags == RI_KEY_MAKE)
                //Input::SetButtonDown(static_cast<uint32>(raw->data.keyboard.VKey));
        }
        else if (raw->header.dwType == RIM_TYPEMOUSE)
        {
            /*Input::SetMouseMoveRelated(raw->data.mouse.lLastX, raw->data.mouse.lLastY);

            if (raw->data.mouse.usButtonFlags & RI_MOUSE_WHEEL)
                Input::SetMouseWheel(raw->data.mouse.usButtonData);

            if (static_cast<uint32>(raw->data.mouse.usButtonFlags) != 0)
                Input::SetMouseFlags(static_cast<uint32>(raw->data.mouse.usButtonFlags));*/
        }
        delete[] lpb;
        return 0;
    }

    case WM_KEYDOWN:
        return 0;

    case WM_KEYUP:
        return 0;

    case WM_SYSKEYDOWN:
        if ((wParam == VK_RETURN) && (lParam & (1 << 29)))
        {
        }
        return 0;

    case WM_EXITSIZEMOVE:
    {
    }
    return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

HWND GetHWND()
{
    return Hwnd;
}

}