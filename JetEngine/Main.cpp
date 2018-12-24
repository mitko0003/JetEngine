#include "Precompiled.h"

#include "RenderDevice.h"
#include "Math.h"

//#define CONSOLE

#if defined(CONSOLE)

int main()
{

}

#else

void MainLoop()
{
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
    {
        PostQuitMessage(0);
    }
    return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
        EndPaint(hwnd, &ps);
    }
    return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

INT WinMain(HINSTANCE instance, HINSTANCE prevInstance, PSTR cmdLine, INT cmdShow)
{
    // Register the window class.
	DebugPrint("Start\n");
    const char CLASS_NAME[] = "Jet Engine";
	
    WNDCLASS wc = { };

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // Create the window.

    HWND hwnd = CreateWindowEx(
                    0,                              // Optional window styles.
                    CLASS_NAME,                     // Window class
                    "[Debug][Vulkan] Jet Engine",   // Window text
                    WS_OVERLAPPEDWINDOW,            // Window style

                    // Size and position
                    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

                    NULL,       // Parent window
                    NULL,       // Menu
                    instance,  // Instance handle
                    NULL        // Additional application data
                );

    if (hwnd == NULL)
    {
        return 0;
    }

	Init(instance, hwnd);
    ShowWindow(hwnd, cmdShow);

    // Run the message loop.

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0))
    {
        MainLoop();
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

#endif