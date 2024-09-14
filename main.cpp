

#include <D3DX12/d3dx12_core.h>
#include <D3DX12/d3dx12_resource_helpers.h>

#include <chrono>
#include <corecrt_wstdio.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <dxgi.h>
#include <dxgi1_3.h>
#include <wrl/client.h>
#include <dwmapi.h>
#include <dxgi1_4.h>
#include <intsafe.h>
#include <D3DX12/d3dx12_barriers.h>
#include <Shlwapi.h>
#include <D3DX12/d3dx12_pipeline_state_stream.h>
#include <D3DX12/d3dx12_root_signature.h>


#include "Helpers.h"

using namespace Microsoft::WRL;
using namespace DirectX;


// Clamp a value between a min and max range.
template<typename T>
constexpr const T& clamp(const T& val, const T& min, const T& max)
{
	return val < min ? min : val > max ? max : val;
}

// Vertex data for a colored cube.
struct VertexPosColor
{
	XMFLOAT3 Position;
	XMFLOAT3 Color;
};

static VertexPosColor g_Vertices[8] = {
	{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) }, // 0
	{ XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) }, // 1
	{ XMFLOAT3(1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) }, // 2
	{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) }, // 3
	{ XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) }, // 4
	{ XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f) }, // 5
	{ XMFLOAT3(1.0f,  1.0f,  1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, // 6
	{ XMFLOAT3(1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f) }  // 7
};

static WORD g_Indicies[36] =
{
	0, 1, 2, 0, 2, 3,
	4, 6, 5, 4, 7, 6,
	4, 5, 1, 4, 1, 0,
	3, 2, 6, 3, 6, 7,
	1, 5, 6, 1, 6, 2,
	4, 0, 3, 4, 3, 7
};


constexpr int numBackBuffers = 3;
constexpr int windowWidth = 700;
constexpr int windowHeight = 700;
constexpr wchar_t wndClassName[] = L"TryIt";

ComPtr<IDXGIAdapter> g_adapter;
ComPtr<ID3D12Device> g_device;
ComPtr<ID3D12CommandQueue> g_commandQueue;
ComPtr<ID3D12GraphicsCommandList> g_commandList;
ComPtr<IDXGISwapChain3> g_swapChain;
ComPtr<ID3D12Resource> g_backBuffers[numBackBuffers];
ComPtr<ID3D12CommandAllocator> g_commandAllocators[numBackBuffers];
ComPtr<ID3D12DescriptorHeap> g_descriptorHeap;
ComPtr<ID3D12Fence> g_fence;
UINT fenceValue = 0;
UINT g_currentBackBuffer = 0;
SIZE_T g_heapSize = 0;
UINT buffersFenceValue[numBackBuffers] = {fenceValue, fenceValue, fenceValue};
bool g_dxInited = false;
std::chrono::time_point<std::chrono::steady_clock> lastTick;
std::chrono::time_point<std::chrono::steady_clock> startPoint;

// used to render cube
ComPtr<ID3D12Resource> g_vertexBuffer;
D3D12_VERTEX_BUFFER_VIEW g_vertexBufferView;

ComPtr<ID3D12Resource> g_indexBuffer;
D3D12_INDEX_BUFFER_VIEW g_indexBufferView;

ComPtr<ID3D12Resource> g_depthBuffer;
ComPtr<ID3D12DescriptorHeap> g_dsvHeap;

ComPtr<ID3D12RootSignature> g_rootSignature;
ComPtr<ID3D12PipelineState> g_pipelineState;

XMMATRIX g_modelMatrix;
XMMATRIX g_viewMatrix;
XMMATRIX g_projectionMatrix;

CD3DX12_VIEWPORT g_viewport;
D3D12_RECT g_d3d12Rect;




LRESULT CALLBACK wWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

ComPtr<IDXGIAdapter> GetAdapter()
{
	ComPtr<IDXGIFactory> factory;
	ThrowIfFailed(CreateDXGIFactory(IID_PPV_ARGS(&factory)));
	ComPtr<IDXGIAdapter> value;
	
	int i = 0;
	SIZE_T maxMemory = 0;
	ComPtr<IDXGIAdapter> adapter;
	ComPtr<IDXGIAdapter1> adapter1;
	while(factory->EnumAdapters(i, &adapter) == S_OK)
	{
		adapter.As(&adapter1);
		DXGI_ADAPTER_DESC1 adapterDesc;
		adapter1.Get()->GetDesc1(&adapterDesc);
		if(adapterDesc.DedicatedVideoMemory > maxMemory)
		{
			maxMemory = adapterDesc.DedicatedVideoMemory;
			value = adapter;
		}
		++i;
	}

	return value;
}

ComPtr<ID3D12Device> CreateDevice(ComPtr<IDXGIAdapter> adapter)
{
	ComPtr<ID3D12Device> value;

	ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&value)));

	return value;
}

ComPtr<ID3D12CommandQueue> CreateCommandQueue(ComPtr<ID3D12Device> device, D3D12_COMMAND_LIST_TYPE type)
{
	ComPtr<ID3D12CommandQueue> value;

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = type;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT;
	queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
	queueDesc.NodeMask = 0;

	ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&value)));

	return value;
}

ComPtr<IDXGISwapChain> CreateSwapChain(HWND hWnd, ComPtr<ID3D12CommandQueue> commandQueue, UINT bufferNum)
{
	DXGI_MODE_DESC modeDesc = {};
	modeDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	DXGI_SWAP_CHAIN_DESC swapChaindesc = {};
	swapChaindesc.BufferDesc = modeDesc;
	swapChaindesc.BufferCount = bufferNum;
	swapChaindesc.OutputWindow = hWnd;
	swapChaindesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChaindesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChaindesc.SampleDesc = { 1, 0 };
	swapChaindesc.Windowed = true;
	swapChaindesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	ComPtr<IDXGIFactory2> factory;
	ComPtr<IDXGISwapChain> value;
	ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory)));
	ThrowIfFailed(factory->CreateSwapChain(commandQueue.Get(), &swapChaindesc, &value));

	return value;
}

ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ComPtr<ID3D12Device> device, D3D12_COMMAND_LIST_TYPE type)
{
	ComPtr<ID3D12CommandAllocator> value;

	ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&value)));

	return value;
}

ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ComPtr<ID3D12CommandAllocator> allocator, ComPtr<ID3D12Device> device, D3D12_COMMAND_LIST_TYPE type)
{
	ComPtr<ID3D12GraphicsCommandList> value;

	ThrowIfFailed(device->CreateCommandList(0, type, allocator.Get(), nullptr, IID_PPV_ARGS(&value)));

	return value;
}

bool updateRenderTarget(ComPtr<ID3D12Device> device, ComPtr<IDXGISwapChain> swapChain, ComPtr<ID3D12Resource> resources[], UINT bufferNum, ComPtr<ID3D12DescriptorHeap> heap, D3D12_DESCRIPTOR_HEAP_TYPE type)
{
	D3D12_RENDER_TARGET_VIEW_DESC rtDesc = {};
	rtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	rtDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	D3D12_CPU_DESCRIPTOR_HANDLE rtHandle = heap->GetCPUDescriptorHandleForHeapStart();

	g_heapSize = device->GetDescriptorHandleIncrementSize(type);

	for(int i = 0; i < bufferNum; ++i)
	{
		ComPtr<ID3D12Resource> buffer;
		swapChain->GetBuffer(i, IID_PPV_ARGS(&buffer));
		device->CreateRenderTargetView(buffer.Get(), &rtDesc, rtHandle);
		resources[i] = buffer;

		rtHandle.ptr += g_heapSize;
	}

	return true;
}

ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device> device, UINT num, D3D12_DESCRIPTOR_HEAP_TYPE type)
{
	ComPtr<ID3D12DescriptorHeap> value;
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = num;
	heapDesc.Type = type;

	ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&value)));

	return value;
}

ComPtr<ID3D12Fence> CreateFence(ComPtr<ID3D12Device> device)
{
	ComPtr<ID3D12Fence> value;
	ThrowIfFailed(device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&value)));

	return value;
}

void UpdateBufferResource(
	ComPtr<ID3D12GraphicsCommandList2> commandList,
	ID3D12Resource** pDestinationResource,
	ID3D12Resource** pIntermediateResource,
	size_t numElements, size_t elementSize, const void* bufferData,
	D3D12_RESOURCE_FLAGS flags, ComPtr<ID3D12Device> device)
{
	size_t bufferSize = numElements * elementSize;

	CD3DX12_HEAP_PROPERTIES destHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC destinationResDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags);
	ThrowIfFailed(device->CreateCommittedResource(
		&destHeapProperties,
		D3D12_HEAP_FLAG_NONE, 
		&destinationResDesc, 
		D3D12_RESOURCE_STATE_COMMON, 
		nullptr, 
		IID_PPV_ARGS(pDestinationResource)));

	if(bufferData)
	{
		CD3DX12_HEAP_PROPERTIES heapPropertiesDesc(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC intermediateResDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
		ThrowIfFailed(device->CreateCommittedResource(
			&heapPropertiesDesc,
			D3D12_HEAP_FLAG_NONE,
			&intermediateResDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(pIntermediateResource)
		));

		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = bufferData;

		UpdateSubresources(commandList.Get(), *pDestinationResource, *pIntermediateResource, 0, 0, 1, &subresourceData);
	}
}

void ResizeDepthBuffer(int width, int height, ComPtr<ID3D12Device> device, ComPtr<ID3D12Resource>& depthBufferResource, ComPtr<ID3D12DescriptorHeap> devHeap)
{
	D3D12_CLEAR_VALUE optimizedClearValue = {};
	optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	optimizedClearValue.DepthStencil = { 1.0f, 0 };

	CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	ThrowIfFailed(device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&optimizedClearValue,
		IID_PPV_ARGS(&depthBufferResource)
	));

	D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
	desc.Format = DXGI_FORMAT_D32_FLOAT;
	desc.Flags = D3D12_DSV_FLAG_NONE;
	desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipSlice = 0;

	auto dsv = devHeap->GetCPUDescriptorHandleForHeapStart();
	device->CreateDepthStencilView(depthBufferResource.Get(), &desc, dsv);
}

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
	// change path to current binary's position
	WCHAR path[MAX_PATH];
	HMODULE hModule = GetModuleHandleW(NULL);
	if(GetModuleFileNameW(hModule, path, MAX_PATH) > 0)
	{
		PathRemoveFileSpecW(path);
		SetCurrentDirectoryW(path);
	}


#if defined(_DEBUG)
	ComPtr<ID3D12Debug> debugController;
	if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
	}
#endif


	// register window
	WNDCLASSW windowClass = {};

	windowClass.style = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW;
	windowClass.lpfnWndProc = wWinProc;
	windowClass.hInstance = hInstance;
	windowClass.lpszClassName = wndClassName;

	if (!RegisterClassW(&windowClass))
	{
		MessageBox(nullptr, L"Failed to register window class!", L"Error", MB_ICONERROR);
		return 0;
	}

	// create window
	HWND hWnd = CreateWindowW(wndClassName, L"WindowOne", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight, nullptr, nullptr, hInstance, nullptr);

	if (hWnd == nullptr)
	{
		DWORD error = GetLastError();
		wchar_t errorMsg[256];
		swprintf_s(errorMsg, L"CreateWindowW failed with error %lu", error);
		MessageBox(nullptr, errorMsg, L"Error", MB_ICONERROR);
		return 0;
	}

	// Show window
	ShowWindow(hWnd, nCmdShow);

	// get adapter
	g_adapter = GetAdapter();

	// create device
	g_device = CreateDevice(g_adapter);

	// create command queue
	g_commandQueue = CreateCommandQueue(g_device, D3D12_COMMAND_LIST_TYPE_DIRECT);

	// create swap chain
	ComPtr<IDXGISwapChain> tempChain = CreateSwapChain(hWnd, g_commandQueue, numBackBuffers);
	ThrowIfFailed(tempChain.As(&g_swapChain));

	// create command allocator
	for(int i = 0; i < numBackBuffers; ++i)
	{
		g_commandAllocators[i] = CreateCommandAllocator(g_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	}

	// create command list
	g_commandList = CreateCommandList(g_commandAllocators[0], g_device, D3D12_COMMAND_LIST_TYPE_DIRECT);

	// create descriptor
	g_descriptorHeap = CreateDescriptorHeap(g_device, numBackBuffers, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// create rt
	updateRenderTarget(g_device, g_swapChain, g_backBuffers, numBackBuffers, g_descriptorHeap, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// create fence
	g_fence = CreateFence(g_device);

	g_currentBackBuffer = g_swapChain->GetCurrentBackBufferIndex();

	lastTick = std::chrono::high_resolution_clock::now();
	startPoint = lastTick;

	
	// create copy list/ allocator/ queue for upload
	ComPtr<ID3D12CommandAllocator> tempAllocator = CreateCommandAllocator(g_device, D3D12_COMMAND_LIST_TYPE_COPY);
	ComPtr<ID3D12GraphicsCommandList2> inList;
	CreateCommandList(tempAllocator, g_device, D3D12_COMMAND_LIST_TYPE_COPY).As(&inList);
	ComPtr<ID3D12CommandQueue> tempQueue = CreateCommandQueue(g_device, D3D12_COMMAND_LIST_TYPE_COPY);
	ComPtr<ID3D12Fence> tempFence = CreateFence(g_device);

	// upload vertex buffer data
	ComPtr<ID3D12Resource> intermediateVertexBuffer;
	UpdateBufferResource(inList, &g_vertexBuffer, &intermediateVertexBuffer, _countof(g_Vertices), sizeof(VertexPosColor), g_Vertices, D3D12_RESOURCE_FLAG_NONE, g_device);
	g_vertexBufferView.BufferLocation = g_vertexBuffer->GetGPUVirtualAddress();
	g_vertexBufferView.SizeInBytes = sizeof(g_Vertices);
	g_vertexBufferView.StrideInBytes = sizeof(VertexPosColor);

	// upload index buffer data
	ComPtr<ID3D12Resource> intermediateIndexBuffer;
	UpdateBufferResource(inList, &g_indexBuffer, &intermediateIndexBuffer, _countof(g_Indicies), sizeof(WORD), g_Indicies, D3D12_RESOURCE_FLAG_NONE, g_device);
	g_indexBufferView.BufferLocation = g_indexBuffer->GetGPUVirtualAddress();
	g_indexBufferView.SizeInBytes = sizeof(g_Indicies);
	g_indexBufferView.Format = DXGI_FORMAT_R16_UINT;


	// create dev heap
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.NumDescriptors = 1;
	ThrowIfFailed(g_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&g_dsvHeap)));

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
	if(FAILED(g_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
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

	ThrowIfFailed(g_device->CreateRootSignature(0, rootSignatureBolb->GetBufferPointer(), rootSignatureBolb->GetBufferSize(), IID_PPV_ARGS(&g_rootSignature)));

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

	pipelineStateStream.pRootSignature = g_rootSignature.Get();
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
	ThrowIfFailed(g_device.As(&device2));
	ThrowIfFailed(device2->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&g_pipelineState)));

	// do upload and wait for fence value
	inList->Close();
	ID3D12CommandList* uploadList[] = { inList.Get() };

	tempQueue->ExecuteCommandLists(1, uploadList);
	tempQueue->Signal(tempFence.Get(), 1);
	if(tempFence->GetCompletedValue() != 1)
	{
		HANDLE uploadEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		tempFence->SetEventOnCompletion(1, uploadEvent);
		WaitForSingleObject(uploadEvent, DWORD_MAX);
	}

	inList.Reset();
	tempAllocator.Reset();
	tempQueue.Reset();
	tempFence.Reset();

	g_viewport = CD3DX12_VIEWPORT(0., 0., windowWidth, windowHeight, D3D12_MIN_DEPTH, D3D12_MAX_DEPTH);
	g_d3d12Rect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);

	ResizeDepthBuffer(windowWidth, windowHeight, g_device, g_depthBuffer, g_dsvHeap);

	g_commandList->Close();

	MSG msg = {};
	//while (PeekMessage(&msg, nullptr, 0, 0, 1))
	while(GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

void RenderCube()
{
	ComPtr<ID3D12Resource> backBuffer = g_backBuffers[g_currentBackBuffer];
	ComPtr<ID3D12CommandAllocator> allocator = g_commandAllocators[g_currentBackBuffer];
	auto rtv = g_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	rtv.ptr += g_currentBackBuffer * g_heapSize;
	auto dsv = g_dsvHeap->GetCPUDescriptorHandleForHeapStart();

	allocator->Reset();
	g_commandList->Reset(allocator.Get(), nullptr);

	// clear render target
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		g_commandList->ResourceBarrier(1, &barrier);

		// clear color
		FLOAT color[4] = { 0.5f, 0.9f, 0.1f, 1 };
		g_commandList->ClearRenderTargetView(rtv, color, 0, nullptr);

		g_commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	}

	g_commandList->SetPipelineState(g_pipelineState.Get());
	g_commandList->SetGraphicsRootSignature(g_rootSignature.Get());

	g_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	g_commandList->IASetIndexBuffer(&g_indexBufferView);
	g_commandList->IASetVertexBuffers(0, 1, &g_vertexBufferView);

	g_commandList->RSSetViewports(1, &g_viewport);
	g_commandList->RSSetScissorRects(1, &g_d3d12Rect);

	g_commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

	// update MVP matrixs
	XMMATRIX mvpMatrix = XMMatrixMultiply(g_modelMatrix, g_viewMatrix);
	mvpMatrix = XMMatrixMultiply(mvpMatrix, g_projectionMatrix);
	g_commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMatrix, 0);

	g_commandList->DrawIndexedInstanced(_countof(g_Indicies), 1, 0, 0, 0);

	// present
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		g_commandList->ResourceBarrier(1, &barrier);
		g_commandList->Close();

		ID3D12CommandList* lists[] = {g_commandList.Get()};
		g_commandQueue->ExecuteCommandLists(1, lists);
		g_commandQueue->Signal(g_fence.Get(), fenceValue);
		buffersFenceValue[g_currentBackBuffer] = fenceValue;
		fenceValue++;

		g_swapChain->Present(1, 0);

		g_currentBackBuffer = g_swapChain->GetCurrentBackBufferIndex();
		UINT waitValue = buffersFenceValue[g_currentBackBuffer];
		if (g_fence->GetCompletedValue() < waitValue)
		{
			HANDLE hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
			g_fence->SetEventOnCompletion(waitValue, hEvent);
			WaitForSingleObject(hEvent, INFINITE);
		}
	}
}


void RenderCleanColor()
{
	// reset command list and command queue
	ComPtr<ID3D12Resource> backBuffer = g_backBuffers[g_currentBackBuffer];
	ComPtr<ID3D12CommandAllocator> allocator = g_commandAllocators[g_currentBackBuffer];
	D3D12_CPU_DESCRIPTOR_HANDLE rtv = g_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	rtv.ptr += g_heapSize * g_currentBackBuffer;

	allocator->Reset();
	g_commandList->Reset(allocator.Get(), nullptr);

	{
		// back buffer present -> render target
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET);

		g_commandList->ResourceBarrier(1, &barrier);
	}

	// clear color
	if(g_currentBackBuffer == 0)
	{
		FLOAT color[4] = { 0.5f, 0.9f, 0.1f, 1 };
		g_commandList->ClearRenderTargetView(rtv, color, 0, nullptr);
	}
	else if(g_currentBackBuffer == 1)
	{
		FLOAT color[4] = { 0.f, 0.f, 0.f, 1 };
		g_commandList->ClearRenderTargetView(rtv, color, 0, nullptr);
	}
	else
	{
		FLOAT color[4] = { 1.f, 1.f, 1.f, 1 };
		g_commandList->ClearRenderTargetView(rtv, color, 0, nullptr);
	}


	{
		// render target -> present
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT);

		g_commandList->ResourceBarrier(1, &barrier);
	}

	g_commandList->Close();
	
	// execute command list
	ID3D12GraphicsCommandList* rawLists =  g_commandList.Get();
	ID3D12CommandList* lists[] = {rawLists};
	g_commandQueue->ExecuteCommandLists(1, lists);
	g_commandQueue->Signal(g_fence.Get(), fenceValue);
	buffersFenceValue[g_currentBackBuffer] = fenceValue;
	fenceValue++;

	// present

	// tearing version
	//g_swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);

	// vSync version
	g_swapChain->Present(1, 0);

	g_currentBackBuffer = g_swapChain->GetCurrentBackBufferIndex();
	UINT waitValue = buffersFenceValue[g_currentBackBuffer];
	if(g_fence->GetCompletedValue() < waitValue)
	{
		HANDLE hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
		g_fence->SetEventOnCompletion(waitValue, hEvent);
		WaitForSingleObject(hEvent, INFINITE);
	}
}



void Update()
{
	auto now = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> elapsed = now - lastTick;
	lastTick = now;
	double fps = 1000 / elapsed.count();
	char buffer[500];
	sprintf_s(buffer, 500, "FPS: %f\n", fps);
	OutputDebugStringA(buffer);

	// for rendering cube
	{
		std::chrono::duration<double, std::milli> totalSecs = now - startPoint;
		float angle = (float)(90 * totalSecs.count() / 1000);
		XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);
		g_modelMatrix = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));

		XMVECTOR eyePosition = XMVectorSet(0, 0, -10, 0);
		XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
		XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);
		g_viewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

		float aspectRatio = windowWidth / static_cast<float>(windowHeight);
		g_projectionMatrix = XMMatrixPerspectiveFovLH(90, aspectRatio, 0.1f, 100.0f);
	}
}

LRESULT wWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_PAINT:
		Update();
		RenderCube();
		//RenderCleanColor();
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}
