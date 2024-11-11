#pragma once

#include <chrono>
#include <dxgi.h>
#include <wrl/client.h>


using Microsoft::WRL::ComPtr;

class Game
{
public:
	Game();
	virtual ~Game();

	virtual void Update();
	virtual void Render() {}

	// input
	virtual void LButtonDown() {}
	virtual void LButtonUp() {}
	virtual void RButtonDown() {}
	virtual void RButtonUp() {}
	virtual void MouseMove() {}

	static LRESULT StaticWinProc(HWND InHwnd, UINT InMessage, WPARAM InWParam, LPARAM InLParam);
	static Game* GlobalGame;

	virtual LRESULT WinProc(HWND InHwnd, UINT InMessage, WPARAM InWParam, LPARAM InLParam);

	void SetShowFps(bool NewShowFps) { _showFps = NewShowFps; }
private:
	HWND _hwnd = nullptr;
	UINT _message = 0;
	WPARAM _wParam = 0;
	LPARAM _lParam = 0;

	bool _showFps = false;
	std::chrono::time_point<std::chrono::steady_clock> _lastTick;
};
