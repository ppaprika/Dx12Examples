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

	static VertexPosColor _vertices[8];
	static WORD _indicies[36];

	virtual void Init() override;
	virtual void Update() override;
	virtual void Render() override;
	virtual LRESULT WinProc(HWND InHwnd, UINT InMessage, WPARAM InWParam, LPARAM InLParam) override;

private:
	struct MouseTracker
	{
		bool mouseLeftButtonDown;
		POINT lastPos;
		POINT deltaPos;
	} mouseTracker = { false, {0, 0}, {0,0} };

	float currentRotX = 0;
	float currentRotY = 0;
	float _fov = 90;

	XMMATRIX g_modelMatrix = {};
	XMMATRIX g_viewMatrix = {};
	XMMATRIX g_projectionMatrix = {};

	ComPtr<ID3D12Resource> _vertexBuffer;
	ComPtr<ID3D12Resource> _indexBuffer;
	D3D12_VERTEX_BUFFER_VIEW _vertexBufferView = {};
	D3D12_INDEX_BUFFER_VIEW _indexBufferView = {};

	ComPtr<ID3D12RootSignature> _rootSignature;
	ComPtr<ID3D12PipelineState> _pipelineState;
};