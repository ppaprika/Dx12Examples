#include "Game.h"
#include <ratio>

void Game::Update()
{
	if(ShowFps)
	{
		auto now = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> elapsed = now - lastTick;
		lastTick = now;
		double fps = 1000 / elapsed.count();
		char buffer[500];
		sprintf_s(buffer, 500, "FPS: %f\n", fps);
	}
}

void Game::InitWinProc(HWND InHwnd, UINT InMessage, WPARAM InWParam, LPARAM InLParam)
{
	Hwnd = InHwnd;
	Message = InMessage;
	WParam = InWParam;
	LParam = InLParam;
}
