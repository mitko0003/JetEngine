#include "Precompiled.h"

#include "RenderDevice-vk.h"
#include "memory.h"
#include "Math.h"
//#include "tiny_obj_loader.h"
//#include <vector>

//#define CONSOLE

#if defined(CONSOLE)

int main()
{

}

#else

void MainLoop(TVulkanAPI *graphicsAPI)
{
	graphicsAPI->HelloWorld();
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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

INT WinMain(HINSTANCE instance, HINSTANCE prevInstance, PSTR cmdLine, INT cmdShow)
{
    // Register the window class.
	char cwd[2048];
	GetCurrentDirectory(sizeof(cwd), cwd);
	DebugPrint<logVerbose>("CWD: %s\n", cwd);
    const char CLASS_NAME[] = "Jet Engine";

	//tinyobj::attrib_t attrib;
	//std::vector<tinyobj::shape_t> shapes;
	//std::vector<tinyobj::material_t> materials;

	//std::string warn;
	//std::string err;
	//(attrib_t *attrib, std::vector<shape_t> *shapes,
	//	std::vector<material_t> *materials, std::string *warn,
	//	std::string *err, std::istream *inStream,
	//	MaterialReader *readMatFn /*= NULL*/, bool triangulate,
	//	bool default_vcols_fallback)
	//bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err);
	
	WNDCLASSEX wc = { };

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = CLASS_NAME;
	wc.hIcon = NULL;

	if (!RegisterClassEx(&wc))
		return false;

    // Create the window.

    HWND hwnd = CreateWindowEx(
        0,                              // Optional window styles.
        CLASS_NAME,                     // Window class
        "[Debug][Vulkan] Jet Engine",   // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        0, 0, 1920, 1080,

        NULL,       // Parent window
        NULL,       // Menu
        instance,   // Instance handle
        nullptr     // Additional application data
    );

    if (hwnd == NULL)
    {
        return 0;
    }

    ShowWindow(hwnd, cmdShow);
	UpdateWindow(hwnd);

	TVulkanAPI vulkan;
	vulkan.Init(instance, hwnd);

    // Run the message loop.
    MSG message = {};
    while (true)
    {
		PeekMessage(&message, NULL, 0, 0, PM_REMOVE);
		switch (message.message)
		{

		}
        MainLoop(&vulkan);
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

	vulkan.Done();
    return 0;
}

#endif