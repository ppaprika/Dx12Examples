#include <windowsx.h>
#include "FPCamera.h"
#include "DirectCommandList.h"
#include "Helpers.h"
#include "Application.h"

#include "IncludePrimitives.h"

void FPCamera::Init()
{
	Game::Init();

	ComPtr<ID3D12Device> device = app_.lock()->GetDevice();

	model_ = std::make_shared<MODELCLASS>(upload_buffer_, direct_command_list_);
	init_ = true;
}

void FPCamera::Update()
{
	Game::Update();

	// update camera
	camera_.Update(GetDeltaTimeInSec(), direct_command_list_->GetTargetWindowWidth() / static_cast<float>(direct_command_list_->GetTargetWindowHeight()));
}

void FPCamera::Render()
{
	Game::Render();

	XMMATRIX view_matrix_ = camera_.GetViewMatrix();
	XMMATRIX projection_matrix_ = camera_.GetProjectionMatrix();

	XMMATRIX mvpMatrix = XMMatrixMultiply(model_matrix_, view_matrix_);
	mvpMatrix = XMMatrixMultiply(mvpMatrix, projection_matrix_);

	model_->UpdateMVPMatrix(mvpMatrix);

	direct_command_list_->Reset();
	direct_command_list_->Draw(model_.get());
	direct_command_list_->Present();
}

LRESULT FPCamera::WinProc(HWND InHwnd, UINT InMessage, WPARAM InWParam, LPARAM InLParam)
{
	if(!init_)
	{
		return Game::WinProc(InHwnd, InMessage, InWParam, InLParam);
	}
	HWND hWnd = direct_command_list_->GetTargetWindow()->GetWindow();


	switch (InMessage)
	{
	case WM_PAINT:
		Update();
		Render();
		return 0;
	case WM_KEYDOWN:
		{
			camera_.OnKeyDown(InWParam);
			return 0;
		}
	case WM_KEYUP:
		{
			camera_.OnKeyUp(InWParam);
			return 0;
		}
	case WM_SIZE:
		{
			if (init_)
			{
				UINT width = LOWORD(InLParam) == 0 ? 1 : LOWORD(InLParam);
				UINT height = HIWORD(InLParam) == 0 ? 1 : HIWORD(InLParam);

				direct_command_list_->GetTargetWindow()->ResizeWindow(width, height);
			}
			return 0;
		}
	case WM_DESTROY:
		PostQuitMessage(WM_QUIT);
		return 0;
	}


	return DefWindowProc(hWnd, InMessage, InWParam, InLParam);
}
