#include "RenderCube.h"

#include <d3dcompiler.h>
#include <windowsx.h>
#include <D3DX12/d3dx12_pipeline_state_stream.h>

#include "Window.h"
#include "Application.h"
#include "DirectCommandList.h"
#include "DirectXTex.h"
#include "Helpers.h"
#include "UploadBuffer.h"

void RenderCube::Init()
{
	Game::Init();

	ComPtr<ID3D12Device> device = app_.lock()->GetDevice();

	cube_ = std::make_shared<SimpleCube>(upload_buffer_, direct_command_list_);
	init_ = true;
}

void RenderCube::Update()
{
	Game::Update();

	current_rot_x_ += mouse_tracker_.delta_pos.y / 5;
	current_rot_y_ += mouse_tracker_.delta_pos.x / 5;

	g_model_matrix_ = XMMatrixRotationAxis({ -1, 0, 0, 0 }, XMConvertToRadians(current_rot_x_));
	g_model_matrix_ = XMMatrixRotationAxis({ 0, -1, 0, 0 }, XMConvertToRadians(current_rot_y_)) * g_model_matrix_;


	XMVECTOR eyePosition = XMVectorSet(0, 0, -10, 0);
	XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
	XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);
	g_view_matrix_ = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

	float aspectRatio = direct_command_list_->GetTargetWindowWidth() / static_cast<float>(direct_command_list_->GetTargetWindowHeight());
	g_projection_matrix_ = XMMatrixPerspectiveFovLH(XMConvertToRadians(fov_), aspectRatio, 0.1f, 100.0f);
}

void RenderCube::Render()
{
	Game::Render();

	XMMATRIX mvpMatrix = XMMatrixMultiply(g_model_matrix_, g_view_matrix_);
	mvpMatrix = XMMatrixMultiply(mvpMatrix, g_projection_matrix_);
	cube_->mvp_matrix = mvpMatrix;
	direct_command_list_->DrawSinglePrimitive(cube_.get());
}

void RenderCube::Release()
{
	Game::Release();
}

LRESULT RenderCube::WinProc(HWND InHwnd, UINT InMessage, WPARAM InWParam, LPARAM InLParam)
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
		//RenderCleanColor();
		return 0;
	case WM_LBUTTONDOWN:
	{
		SetCapture(hWnd);
		mouse_tracker_.mouse_left_button_down = true;
		mouse_tracker_.last_pos.x = GET_X_LPARAM(InLParam);
		mouse_tracker_.last_pos.y = GET_Y_LPARAM(InLParam);


		//char buffer[] = "Mouse Button Down.\n";
		//OutputDebugStringA(buffer);
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

			//char buffer[200];
			//sprintf_s(buffer, "deltaX:%d  deltaY:%d \n", mouse_tracker_.delta_pos.x, mouse_tracker_.delta_pos.y);
			//OutputDebugStringA(buffer);
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


		//char buffer[] = "Mouse Button Up.\n";
		//OutputDebugStringA(buffer);
		break;
	}
	case WM_MOUSEWHEEL:
	{
		float zDelta = ((int)(short)HIWORD(InWParam)) / (float)WHEEL_DELTA;
		fov_ -= zDelta;
		fov_ = clamp(fov_, 12.0f, 90.0f);

		//char buffer[500];
		//sprintf_s(buffer, "g_fov: %f \n", fov_);
		//OutputDebugStringA(buffer);
		break;
	}
	case WM_SIZE:
	{
		if (init_)
		{
			UINT width = LOWORD(InLParam) == 0 ? 1 : LOWORD(InLParam);
			UINT height = HIWORD(InLParam) == 0 ? 1 : HIWORD(InLParam);

			//char buffer[500];
			//sprintf_s(buffer, "width: %d, height: %d \n", width, height);
			//OutputDebugStringA(buffer);
			direct_command_list_->GetTargetWindow()->UpdateSize(width, height);
		}
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(WM_QUIT);
		break;
	default:
		return DefWindowProc(hWnd, InMessage, InWParam, InLParam);
	}
	return DefWindowProc(hWnd, InMessage, InWParam, InLParam);
}
