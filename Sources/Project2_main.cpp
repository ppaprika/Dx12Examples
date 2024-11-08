#define WIN32_LEAN_AND_MEAN

#include <dxgi1_3.h>

#include "Helpers.h"

#include <dxgidebug.h>
#include <float.h>
#include <Shlwapi.h>

void ReportLiveObjects()
{
	IDXGIDebug1* dxgiDebug;
	DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));

	dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
	dxgiDebug->Release();
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    int retCode = 0;

    // Set the working directory to the path of the executable.
    WCHAR path[MAX_PATH];
    HMODULE hModule = GetModuleHandleW(nullptr);
    if (GetModuleFileNameW(hModule, path, MAX_PATH) > 0)
    {
        PathRemoveFileSpecW(path);
        SetCurrentDirectoryW(path);
    }

    // register window
    constexpr wchar_t CLASS_NAME[] = L"Dx12Example";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    // create window
    HWND hwnd = CreateWindowEx(
		0,
        CLASS_NAME,
        L"Window_1",
        WS_OVERLAPPEDWINDOW,
        600, 600, 600, 600,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if(hwnd == nullptr)
    {
        ShowLastError();
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);






    MSG msg = {};
    while(GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
        // Add other message handlers here if needed
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}