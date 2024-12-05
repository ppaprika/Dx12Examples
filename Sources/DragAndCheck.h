#pragma once

#include <DirectXMath.h>

#include "Game.h"
#include "InstancedSimpleCube.h"
#include "SimpleCube.h"
#include "TextureLoader.h"

using namespace DirectX;

#define MODELCLASS InstancedSimpleCube

class DragAndCheck : public Game
{
public:
	virtual void Init() override;
	virtual void Update() override;
	virtual void Render() override;
	virtual void Release() override;
	virtual LRESULT WinProc(HWND InHwnd, UINT InMessage, WPARAM InWParam, LPARAM InLParam) override;

protected:
	struct MouseTracker
	{
		bool mouse_left_button_down;
		POINT last_pos;
		POINT delta_pos;
	} mouse_tracker_ = { false, {0, 0}, {0,0} };

	float current_rot_x_ = 0;
	float current_rot_y_ = 0;
	float fov_ = 90;

	XMMATRIX g_model_matrix_ = {};
	XMMATRIX g_view_matrix_ = {};
	XMMATRIX g_projection_matrix_ = {};

	std::shared_ptr<MODELCLASS> cube_;

	bool init_ = false;
};