#include "Precompiled.h"

#include "WindowContext-nt.h"

LRESULT CALLBACK WindowMessageHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_SIZE:
    case WM_EXITSIZEMOVE: {
        PostMessage(hwnd, WM_USER + 1, wParam, lParam);
    } return 0;

    case WM_DESTROY: {
        PostQuitMessage(0);
    } return 0;

    default: return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

TWindowNT::TWindowNT(HINSTANCE instance, INT nCmdShow) :
    Instance(instance)
{
    WNDCLASSEX windowClassEx = {};
    windowClassEx.cbSize = sizeof(WNDCLASSEX);
    windowClassEx.style = CS_HREDRAW | CS_VREDRAW;
    windowClassEx.lpfnWndProc = WindowMessageHandler;
    windowClassEx.hInstance = Instance;
    windowClassEx.lpszClassName = TEXT("MainWindowClass");
    windowClassEx.hIcon = NULL;

    Class = RegisterClassEx(&windowClassEx);
    if (Class == NULL)
        return;

    // Create the window.

    Handle = CreateWindowEx(
        0,                                  // Optional window styles.
        MAKEINTATOM(Class),       // Window class
        TEXT("[Debug][Vulkan] Jet Engine"), // Window text
        WS_OVERLAPPEDWINDOW,                // Window style

        // Size and position
        0, 0, 1920, 1080,

        NULL,       // Parent window
        NULL,       // Menu
        Instance,   // Instance handle
        nullptr     // Additional application data
    );

    ShowWindow(Handle, nCmdShow);
    UpdateWindow(Handle);
}

TWindowNT::~TWindowNT()
{
    CloseHandle(Handle);
    UnregisterClass(MAKEINTATOM(Class), Instance);
}

TPair<int32, int32> TWindowNT::GetDimensions() const
{
    RECT windowRect;
    BOOL success = GetWindowRect(Handle, &windowRect);
    ASSERT(success); success;
    return MakePair(
        int32(windowRect.right - windowRect.left),
        int32(windowRect.bottom - windowRect.top)
    );
}

void TWindowNT::SetTitle(const char *title) const
{
    SetWindowText(Handle, title);
}