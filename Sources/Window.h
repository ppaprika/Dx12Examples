#pragma once
#include "CommandList.h"


class Window
{
public:
	Window(WNDPROC winProc, HINSTANCE hInstance, LPCWSTR wndClassName, LPCWSTR wndName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, int nCmdShow);
	virtual ~Window();

	HWND GetWindow() { return _window; }
private:
	HWND _window;
};
