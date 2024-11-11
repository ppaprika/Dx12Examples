#include "Game.h"

#include <codecvt>
#include <ratio>

Game* Game::GlobalGame = nullptr;

Game::Game()
{
}

Game::~Game()
{
}

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

LRESULT Game::StaticWinProc(HWND InHwnd, UINT InMessage, WPARAM InWParam, LPARAM InLParam)
{
	return GlobalGame->WinProc(InHwnd, InMessage, InWParam, InLParam);
}

LRESULT Game::WinProc(HWND InHwnd, UINT InMessage, WPARAM InWParam, LPARAM InLParam)
{
	_hwnd = InHwnd;
	_message = InMessage;
	_wParam = InWParam;
	_lParam = InLParam;

	return DefWindowProc(InHwnd, InMessage, InWParam, InLParam);
}