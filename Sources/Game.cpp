#include "Game.h"
#include <ratio>

void Game::Update()
{
	if(_showFps)
	{
		auto now = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> elapsed = now - _lastTick;
		_lastTick = now;
		double fps = 1000 / elapsed.count();
		char buffer[500];
		sprintf_s(buffer, 500, "FPS: %f\n", fps);
	}
}

void Game::InitWinProc(HWND InHwnd, UINT InMessage, WPARAM InWParam, LPARAM InLParam)
{
	_hwnd = InHwnd;
	_message = InMessage;
	_wParam = InWParam;
	_lParam = InLParam;
}
