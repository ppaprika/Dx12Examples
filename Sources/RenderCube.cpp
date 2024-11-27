#include "RenderCube.h"

#include <d3dcompiler.h>
#include <windowsx.h>
#include <D3DX12/d3dx12_barriers.h>
#include <D3DX12/d3dx12_pipeline_state_stream.h>
#include <D3DX12/d3dx12_root_signature.h>

#include "Window.h"
#include "Application.h"
#include "CommandList.h"
#include "DirectXTex.h"
#include "Helpers.h"
#include "UploadBuffer.h"

RenderCube::VertexPosColor RenderCube::vertices[8] = {
	{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) }, // 0
	{ XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) }, // 1
	{ XMFLOAT3(1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) }, // 2
	{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) }, // 3
	{ XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) }, // 4
	{ XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f) }, // 5
	{XMFLOAT3(1.0f,  1.0f,  1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, // 6
	{ XMFLOAT3(1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f) }  // 7
};

WORD RenderCube::indicies[36] =
{
	0, 1, 2, 0, 2, 3,
	4, 6, 5, 4, 7, 6,
	4, 5, 1, 4, 1, 0,
	3, 2, 6, 3, 6, 7,
	1, 5, 6, 1, 6, 2,
	4, 0, 3, 4, 3, 7
};

void RenderCube::Init()
{
	Game::Init();

	ComPtr<ID3D12Device> device = app_.lock()->GetDevice();

	vertex_buffer_view_.Init(sizeof(vertices), 64, sizeof(VertexPosColor), vertices, upload_buffer_);
	index_buffer_view_.Init(sizeof(indicies), 0, DXGI_FORMAT_R16_UINT, indicies, upload_buffer_);

	ComPtr<ID3DBlob> vertexShader;
	ThrowIfFailed(D3DReadFileToBlob(L"VertexShader.cso", &vertexShader));

	ComPtr<ID3DBlob> pixelShader;
	ThrowIfFailed(D3DReadFileToBlob(L"PixelShader.cso", &pixelShader));

	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{"MYPOSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"MYCOLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	CD3DX12_ROOT_PARAMETER1 rootParameter[1];
	rootParameter[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription = {};
	rootSignatureDescription.Init_1_1(_countof(rootParameter), rootParameter, 0, nullptr, rootSignatureFlags);

	ComPtr<ID3DBlob> rootSignatureBolb;
	ComPtr<ID3DBlob> errorBold;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription, featureData.HighestVersion, &rootSignatureBolb, &errorBold));

	ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBolb->GetBufferPointer(), rootSignatureBolb->GetBufferSize(), IID_PPV_ARGS(&root_signature_)));

	struct PipelineStateStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopology;
		CD3DX12_PIPELINE_STATE_STREAM_VS VS;
		CD3DX12_PIPELINE_STATE_STREAM_PS PS;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
	} pipelineStateStream;

	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	rtvFormats.NumRenderTargets = 1;
	rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	pipelineStateStream.pRootSignature = root_signature_.Get();
	pipelineStateStream.InputLayout = { inputLayout, _countof(inputLayout) };
	pipelineStateStream.PrimitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
	pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
	pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	pipelineStateStream.RTVFormats = rtvFormats;

	D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {};
	pipelineStateStreamDesc.pPipelineStateSubobjectStream = &pipelineStateStream;
	pipelineStateStreamDesc.SizeInBytes = sizeof(PipelineStateStream);

	ComPtr<ID3D12Device2> device2;
	ThrowIfFailed(device.As(&device2));
	ThrowIfFailed(device2->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&pipeline_state_)));

	init_ = true;

	//ScratchImage scratchImage;
	//HRESULT hr = LoadFromWICFile(L"../../resources/Texture.jpg", WIC_FLAGS_NONE, nullptr, scratchImage);
	//if (FAILED(hr)) return;
	//const Image* img = scratchImage.GetImage(0, 0, 0);
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

	float aspectRatio = window_->GetWidth() / static_cast<float>(window_->GetHeight());
	g_projection_matrix_ = XMMatrixPerspectiveFovLH(XMConvertToRadians(fov_), aspectRatio, 0.1f, 100.0f);
}

void RenderCube::Render()
{
	Game::Render();

	DrawWindowParams Params = {};
	Params.IndexBufferView = index_buffer_view_.GetIndexBufferView();
	Params.VertexBufferView = vertex_buffer_view_.GetVertexBufferView();
	Params.DrawNum = _countof(indicies);
	Params.PSO = pipeline_state_;
	Params.RootSignature = root_signature_;
	Params.SetRootConstant = [this](ComPtr<ID3D12GraphicsCommandList2> CommandList)
		{
			XMMATRIX mvpMatrix = XMMatrixMultiply(g_model_matrix_, g_view_matrix_);
			mvpMatrix = XMMatrixMultiply(mvpMatrix, g_projection_matrix_);
			CommandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMatrix, 0);
		};

	window_->_commandList->DrawToWindow(window_, Params);
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
	

	HWND hWnd = window_->GetWindow();

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
			UINT numBackBuffers = window_->_numOfBackBuffers;
			UINT windowWidth = window_->GetWidth();
			UINT windowHeight = window_->GetHeight();
			std::vector<ComPtr<ID3D12Resource>>& backBuffers = window_->_backBuffers;
			ComPtr<IDXGISwapChain3> swapChain = window_->_swapChain;
			ComPtr<ID3D12Device> device = GetDevice();
			ComPtr<ID3D12DescriptorHeap> descriptorHeap = window_->_descriptorHeap;
			UINT& currentBackBuffer = window_->_currentBackBuffer;

			UINT width = LOWORD(InLParam) == 0 ? 1 : LOWORD(InLParam);
			UINT height = HIWORD(InLParam) == 0 ? 1 : HIWORD(InLParam);

			char buffer[500];
			sprintf_s(buffer, "width: %d, height: %d \n", width, height);
			OutputDebugStringA(buffer);

			window_->UpdateSize(width, height);

			window_->SetHeight(height);
			window_->SetWidth(width);

			window_->InitViewportAndRect();
			window_->Flush();

			// resize depth buffer
			window_->ResizeDepthBuffer();

			// resize render target view
			for (int i = 0; i < numBackBuffers; ++i)
			{
				backBuffers[i].Reset();
			}
			swapChain->ResizeBuffers(numBackBuffers, windowWidth, windowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
			window_->UpdateRenderTarget(device, swapChain, backBuffers, numBackBuffers, descriptorHeap, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, window_->_heapSize);
			currentBackBuffer = swapChain->GetCurrentBackBufferIndex();
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
