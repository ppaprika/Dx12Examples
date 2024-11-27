#pragma once

#include <DirectXMath.h>

#include "Game.h"

using namespace DirectX;

class RenderCube : public Game
{
public:
	struct VertexPosColor
	{
		XMFLOAT3 Position;
		XMFLOAT3 Color;
	};

	static VertexPosColor vertices[8];
	static WORD indicies[36];

	virtual void Init() override;
	virtual void Update() override;
	virtual void Render() override;
	virtual void Release() override;
	virtual LRESULT WinProc(HWND InHwnd, UINT InMessage, WPARAM InWParam, LPARAM InLParam) override;

private:
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

	VertexBufferView vertex_buffer_view_;
	IndexBufferView index_buffer_view_;

	ComPtr<ID3D12RootSignature> root_signature_;
	ComPtr<ID3D12PipelineState> pipeline_state_;

	bool init_ = false;
};