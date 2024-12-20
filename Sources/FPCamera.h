#pragma once
#include <DirectXMath.h>

#include "Camera.h"
#include "Game.h"

class InstancedSimpleCube;
class SimpleCube;

#define MODELCLASS SimpleCube

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
	XMMATRIX model_matrix_ = DirectX::XMMatrixIdentity();

	Camera camera_;

	std::shared_ptr<MODELCLASS> model_;

	bool init_ = false;
};

