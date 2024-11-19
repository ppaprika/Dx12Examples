#include "RenderCube.h"

#include <d3dcompiler.h>
#include <windowsx.h>
#include <D3DX12/d3dx12_barriers.h>
#include <D3DX12/d3dx12_pipeline_state_stream.h>
#include <D3DX12/d3dx12_root_signature.h>

#include "Window.h"
#include "Application.h"
#include "CommandList.h"
#include "Helpers.h"
#include "UploadBuffer.h"

RenderCube::VertexPosColor RenderCube::_vertices[8] = {
	{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) }, // 0
	{ XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) }, // 1
	{ XMFLOAT3(1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) }, // 2
	{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) }, // 3
	{ XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) }, // 4
	{ XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f) }, // 5
	{XMFLOAT3(1.0f,  1.0f,  1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, // 6
	{ XMFLOAT3(1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f) }  // 7
};

WORD RenderCube::_indicies[36] =
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

	ComPtr<ID3D12Device> device = _app.lock()->GetDevice();


	//_uploadBuffer->DoUpload(_vertexBuffer, _countof(_vertices), sizeof(_vertices), _vertices, D3D12_RESOURCE_FLAG_NONE);
	UploadBuffer::Memory memory = _uploadBuffer->Allocation(sizeof(_vertices));
	memcpy(memory.CPUPtr, _vertices, sizeof(_vertices));
	_vertexBufferView.BufferLocation = memory.GPUPtr;
	_vertexBufferView.SizeInBytes = sizeof(_vertices);
	_vertexBufferView.StrideInBytes = sizeof(VertexPosColor);

	//_uploadBuffer->DoUpload(_indexBuffer, _countof(_indicies), sizeof(_indicies), _indicies, D3D12_RESOURCE_FLAG_NONE);
	memory = _uploadBuffer->Allocation(sizeof(_indicies));
	memcpy(memory.CPUPtr, _indicies, sizeof(_indicies));
	_indexBufferView.BufferLocation = memory.GPUPtr;
	_indexBufferView.SizeInBytes = sizeof(_indicies);
	_indexBufferView.Format = DXGI_FORMAT_R16_UINT;

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

	ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBolb->GetBufferPointer(), rootSignatureBolb->GetBufferSize(), IID_PPV_ARGS(&_rootSignature)));

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

	pipelineStateStream.pRootSignature = _rootSignature.Get();
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
	ThrowIfFailed(device2->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&_pipelineState)));

	_init = true;
}

void RenderCube::Update()
{
	Game::Update();

	currentRotX += mouseTracker.deltaPos.y / 5;
	currentRotY += mouseTracker.deltaPos.x / 5;

	g_modelMatrix = XMMatrixRotationAxis({ -1, 0, 0, 0 }, XMConvertToRadians(currentRotX));
	g_modelMatrix = XMMatrixRotationAxis({ 0, -1, 0, 0 }, XMConvertToRadians(currentRotY)) * g_modelMatrix;


	XMVECTOR eyePosition = XMVectorSet(0, 0, -10, 0);
	XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
	XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);
	g_viewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

	float aspectRatio = _window->GetWidth() / static_cast<float>(_window->GetHeight());
	g_projectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(_fov), aspectRatio, 0.1f, 100.0f);
}

void RenderCube::Render()
{
	Game::Render();

	CD3DX12_VIEWPORT& viewport = _window->_viewport;
	D3D12_RECT& d3d12Rect = _window->_d3d12Rect;

	DrawWindowParams Params = {};
	Params.IndexBufferView = _indexBufferView;
	Params.VertexBufferView = _vertexBufferView;
	Params.DrawNum = _countof(_indicies);
	Params.PSO = _pipelineState;
	Params.RootSignature = _rootSignature;
	Params.SetRootConstant = [this](ComPtr<ID3D12GraphicsCommandList2> CommandList)
		{
			XMMATRIX mvpMatrix = XMMatrixMultiply(g_modelMatrix, g_viewMatrix);
			mvpMatrix = XMMatrixMultiply(mvpMatrix, g_projectionMatrix);
			CommandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMatrix, 0);
		};

	_window->_commandList->DrawToWindow(_window, Params);
}

LRESULT RenderCube::WinProc(HWND InHwnd, UINT InMessage, WPARAM InWParam, LPARAM InLParam)
{
	if(!_init)
	{
		return Game::WinProc(InHwnd, InMessage, InWParam, InLParam);
	}
	

	HWND hWnd = _window->GetWindow();

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
		mouseTracker.mouseLeftButtonDown = true;
		mouseTracker.lastPos.x = GET_X_LPARAM(InLParam);
		mouseTracker.lastPos.y = GET_Y_LPARAM(InLParam);


		char buffer[] = "Mouse Button Down.\n";
		OutputDebugStringA(buffer);
		break;
	}
	case WM_MOUSEMOVE:
	{
		if (mouseTracker.mouseLeftButtonDown)
		{
			mouseTracker.deltaPos.x = GET_X_LPARAM(InLParam) - mouseTracker.lastPos.x;
			mouseTracker.deltaPos.y = GET_Y_LPARAM(InLParam) - mouseTracker.lastPos.y;

			mouseTracker.lastPos.x = GET_X_LPARAM(InLParam);
			mouseTracker.lastPos.y = GET_Y_LPARAM(InLParam);

			char buffer[200];
			sprintf_s(buffer, "deltaX:%d  deltaY:%d \n", mouseTracker.deltaPos.x, mouseTracker.deltaPos.y);
			OutputDebugStringA(buffer);
		}
		break;
	}
	case WM_LBUTTONUP:
	{
		if (GetCapture() == hWnd)
		{
			ReleaseCapture();
		}
		mouseTracker.mouseLeftButtonDown = false;
		mouseTracker.deltaPos = { 0, 0 };


		char buffer[] = "Mouse Button Up.\n";
		OutputDebugStringA(buffer);
		break;
	}
	case WM_MOUSEWHEEL:
	{
		float zDelta = ((int)(short)HIWORD(InWParam)) / (float)WHEEL_DELTA;
		_fov += zDelta;
		_fov = clamp(_fov, 12.0f, 90.0f);

		char buffer[500];
		sprintf_s(buffer, "g_fov: %f \n", _fov);
		OutputDebugStringA(buffer);
		break;
	}
	case WM_SIZE:
	{
		if (_init)
		{
			UINT numBackBuffers = _window->_numOfBackBuffers;
			UINT windowWidth = _window->GetWidth();
			UINT windowHeight = _window->GetHeight();
			std::vector<ComPtr<ID3D12Resource>>& backBuffers = _window->_backBuffers;
			ComPtr<IDXGISwapChain3> swapChain = _window->_swapChain;
			ComPtr<ID3D12Device> device = GetDevice();
			ComPtr<ID3D12DescriptorHeap> descriptorHeap = _window->_descriptorHeap;
			UINT& currentBackBuffer = _window->_currentBackBuffer;




			UINT width = LOWORD(InLParam) == 0 ? 1 : LOWORD(InLParam);
			UINT height = HIWORD(InLParam) == 0 ? 1 : HIWORD(InLParam);

			char buffer[500];
			sprintf_s(buffer, "width: %d, height: %d \n", width, height);
			OutputDebugStringA(buffer);

			_window->UpdateSize(width, height);

			_window->SetHeight(height);
			_window->SetWidth(width);

			_window->InitViewportAndRect();
			_window->Flush();

			// resize depth buffer
			_window->ResizeDepthBuffer();

			// resize render target view
			for (int i = 0; i < numBackBuffers; ++i)
			{
				backBuffers[i].Reset();
			}
			swapChain->ResizeBuffers(numBackBuffers, windowWidth, windowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
			_window->UpdateRenderTarget(device, swapChain, backBuffers, numBackBuffers, descriptorHeap, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, _window->_heapSize);
			currentBackBuffer = swapChain->GetCurrentBackBufferIndex();
		}
		break;
	}
	case WM_DESTROY:
		//Quit();
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hWnd, InMessage, InWParam, InLParam);
	}
}
