#include "Window.h"

Window::Window(WNDPROC winProc, HINSTANCE hInstance, LPCWSTR wndClassName, LPCWSTR wndName, DWORD dwStyle, int x, int y,
	int nWidth, int nHeight, int nCmdShow)
{
	WNDCLASSW windowClass = {};
	windowClass.style = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW;
	windowClass.lpfnWndProc = winProc;
	windowClass.hInstance = hInstance;
	windowClass.lpszClassName = wndClassName;

	if(!RegisterClassW(&windowClass))
	{
		MessageBox(nullptr, L"Failed to register window class!", L"Error", MB_ICONERROR);
		ExitProcess(1);
	}

	_window = CreateWindowW(wndClassName, wndName, dwStyle, x, y, nWidth, nHeight, nullptr, nullptr, hInstance, nullptr);
	if(_window == nullptr)
	{
		DWORD error = GetLastError();
		wchar_t errorMsg[256];
		swprintf_s(errorMsg, L"CreateWindowW failed with error %lu", error);
		MessageBox(nullptr, errorMsg, L"Error", MB_ICONERROR);
		ExitProcess(1);
	}

	ShowWindow(_window, nCmdShow);
}

Window::~Window()
{
}
