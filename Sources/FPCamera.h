#pragma once
#include <DirectXMath.h>

#include "Game.h"

class InstancedSimpleCube;
class SimpleCube;

#define MODELCLASS InstancedSimpleCube

using DirectX::XMMATRIX;
using DirectX::XMVECTOR;

class FPCamera : public Game
{
public:

	virtual void Init() override;
	virtual void Update() override;
	virtual void Render() override;
	virtual LRESULT WinProc(HWND InHwnd, UINT InMessage, WPARAM InWParam, LPARAM InLParam) override;
private:
	struct MouseTracker
	{
		bool mouse_left_button_down;
		POINT last_pos;
		POINT delta_pos;
	} mouse_tracker_ = { false, {0, 0}, {0,0} };

	XMVECTOR camera_pos_ = {0, 0, -10, 0};
	XMVECTOR camera_forward_ = {0, 0, 1, 0};
	XMVECTOR camera_up_vec_ = {0, 1, 0, 0 };
	float camera_speed_ = 0.1;

	XMMATRIX model_matrix_ = DirectX::XMMatrixIdentity();
	XMMATRIX view_matrix_ = {};
	XMMATRIX projection_matrix_ = {};

	std::shared_ptr<MODELCLASS> model_;

	bool init_ = false;
	float fov_ = 90;
};

