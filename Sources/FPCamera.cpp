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

	float cameraRotX = static_cast<float>(mouse_tracker_.delta_pos.x) / 5;
	float cameraRotY = static_cast<float>(mouse_tracker_.delta_pos.y) / 5;

	XMMATRIX cameraRotMatrix = XMMatrixRotationAxis({ -1, 0, 0, 0 }, XMConvertToRadians(cameraRotY));
	cameraRotMatrix = XMMatrixRotationAxis({ 0, -1, 0, 0 }, XMConvertToRadians(cameraRotX)) * cameraRotMatrix;
	camera_forward_ = XMVector4Transform(camera_forward_, cameraRotMatrix);
	camera_forward_ = XMVector4Normalize(camera_forward_);

	view_matrix_ = XMMatrixLookAtLH(camera_pos_, camera_forward_ + camera_pos_, camera_up_vec_);

	float aspectRatio = direct_command_list_->GetTargetWindowWidth() / static_cast<float>(direct_command_list_->GetTargetWindowHeight());
	projection_matrix_ = XMMatrixPerspectiveFovLH(XMConvertToRadians(fov_), aspectRatio, 0.1f, 100.0f);

	XMMATRIX mvpMatrix = XMMatrixMultiply(model_matrix_, view_matrix_);
	mvpMatrix = XMMatrixMultiply(mvpMatrix, projection_matrix_);
}

void FPCamera::Render()
{
	Game::Render();

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
	case WM_LBUTTONDOWN:
		{
			SetCapture(hWnd);
			mouse_tracker_.mouse_left_button_down = true;
			mouse_tracker_.last_pos.x = GET_X_LPARAM(InLParam);
			mouse_tracker_.last_pos.y = GET_Y_LPARAM(InLParam);
			break;
		}
	case WM_MOUSEMOVE:
		{
			if (mouse_tracker_.mouse_left_button_down)
			{
				mouse_tracker_.delta_pos.x = GET_X_LPARAM(InLParam) - mouse_tracker_.last_pos.x;
				mouse_tracker_.delta_pos.y = GET_Y_LPARAM(InLParam) - mouse_tracker_.last_pos.y;

				mouse_tracker_.last_pos.x = GET_X_LPARAM(InLParam);
				mouse_tracker_.last_pos.y = GET_Y_LPARAM(InLParam);
			}
			break;
		}
	case WM_LBUTTONUP:
		{
			if (GetCapture() == hWnd)
			{
				ReleaseCapture();
			}
			mouse_tracker_.mouse_left_button_down = false;
			mouse_tracker_.delta_pos = { 0, 0 };

			break;
		}
	case WM_MOUSEWHEEL:
		{
			float zDelta = ((int)(short)HIWORD(InWParam)) / (float)WHEEL_DELTA;
			fov_ -= zDelta;
			fov_ = clamp(fov_, 12.0f, 90.0f);
			break;
		}
	case WM_KEYDOWN:
		{
			WORD vkCode = LOWORD(InWParam);

			switch(vkCode)
			{
			case 'W':
				camera_pos_ += camera_forward_ * camera_speed_;
				break;
			case 's':
				camera_pos_ -= camera_forward_ * camera_speed_;
				break;
			}
			break;
		}
	case WM_SIZE:
		{
			if (init_)
			{
				UINT width = LOWORD(InLParam) == 0 ? 1 : LOWORD(InLParam);
				UINT height = HIWORD(InLParam) == 0 ? 1 : HIWORD(InLParam);

				direct_command_list_->GetTargetWindow()->ResizeWindow(width, height);
			}
			break;
		}
	case WM_DESTROY:
		PostQuitMessage(WM_QUIT);
		break;
	default:
		return DefWindowProc(hWnd, InMessage, InWParam, InLParam);
	}


	return Game::WinProc(InHwnd, InMessage, InWParam, InLParam);
}
