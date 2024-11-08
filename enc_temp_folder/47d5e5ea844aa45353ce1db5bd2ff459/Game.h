#pragma once

#include <chrono>
#include "Helpers.h"

class Game
{
public:
	Game() = default;
	virtual ~Game() = default;

	virtual void Update();
	virtual void Render() {}

	// input
	virtual void LButtonDown() {}
	virtual void LButtonUp() {}
	virtual void RButtonDown() {}
	virtual void RButtonUp() {}
	virtual void MouseMove() {}

	virtual void InitWinProc(HWND InHwnd, UINT InMessage, WPARAM InWParam, LPARAM InLParam);

	void SetShowFps(bool NewShowFps) { ShowFps = NewShowFps; }
private:
	HWND Hwnd = nullptr;
	UINT Message = 0;
	WPARAM WParam = 0;
	LPARAM LParam = 0;

	bool ShowFps = false;
	std::chrono::time_point<std::chrono::steady_clock> lastTick;
};
