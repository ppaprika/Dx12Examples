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
	{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(0, 1)}, // 0
	{ XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f),  XMFLOAT2(0, 0) }, // 1
	{ XMFLOAT3(1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(1, 0) }, // 2
	{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) , XMFLOAT2(1, 1)}, // 3
	{ XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(0, 1) }, // 4
	{ XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f) , XMFLOAT2(0, 0)}, // 5
	{XMFLOAT3(1.0f,  1.0f,  1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) , XMFLOAT2(1, 0)}, // 6
	{ XMFLOAT3(1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f) , XMFLOAT2(1, 1)}  // 7
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
		{"MYCOLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"MYTEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// create root signature
	CD3DX12_DESCRIPTOR_RANGE1 descriptorRange[1] = {};
	descriptorRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_NONE);

	CD3DX12_ROOT_PARAMETER1 rootParameter[2] = {};
	rootParameter[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	rootParameter[1].InitAsDescriptorTable(1, &descriptorRange[0], D3D12_SHADER_VISIBILITY_PIXEL);
	// inline descriptor
	//rootParameter[1].InitAsShaderResourceView(0);

	// create static sampler
	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.ShaderRegister = 0;
	samplerDesc.RegisterSpace = 0;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription = {};
	rootSignatureDescription.Init_1_1(_countof(rootParameter), rootParameter, 1, &samplerDesc, rootSignatureFlags);

	ComPtr<ID3DBlob> rootSignatureBlob;
	ComPtr<ID3DBlob> errorBlob;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription, featureData.HighestVersion, &rootSignatureBlob, &errorBlob));

	ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&root_signature_)));

	// create PSO
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

	// load texture to srv
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srv_desc_heap_)));
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = srv_desc_heap_->GetCPUDescriptorHandleForHeapStart();
	TextureLoader::LoadTextureFromFile(Application::GetDevice(), direct_command_list_->GetCommandList(), L"../../resources/Texture.jpg", texture_resource_, srvHandle);
	direct_command_list_->SingleAndWait();

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
		// inline constant
			CommandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMatrix, 0);
		// descriptor table
			CommandList->SetDescriptorHeaps(1, srv_desc_heap_.GetAddressOf());
			CommandList->SetGraphicsRootDescriptorTable(1, srv_desc_heap_->GetGPUDescriptorHandleForHeapStart());

		// inline descriptor
		// CommandList->SetGraphicsRootShaderResourceView(1, texture_resource_.texture->GetGPUVirtualAddress());
		// should use
		// ByteAddressBuffer texture : register(t0);
		// instead of
		// Texture2D texture : register(t0);
		// in pixel shader
		};

	direct_command_list_->DrawToWindow(window_, Params);
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
			UINT width = LOWORD(InLParam) == 0 ? 1 : LOWORD(InLParam);
			UINT height = HIWORD(InLParam) == 0 ? 1 : HIWORD(InLParam);

			//char buffer[500];
			//sprintf_s(buffer, "width: %d, height: %d \n", width, height);
			//OutputDebugStringA(buffer);
			window_->UpdateSize(width, height);
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
