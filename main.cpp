//#define WIN32_LEAN_AND_MEAN
//#include <Windows.h>
//#include <Shlwapi.h>
//
//#include "Application.h"
//#include "RenderCube.h"
//
//#include <dxgidebug.h>
//
//void ReportLiveObjects()
//{
//    IDXGIDebug1* dxgiDebug;
//    DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));
//
//    dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
//    dxgiDebug->Release();
//}
//
//int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
//{
//    int retCode = 0;
//
//    // Set the working directory to the path of the executable.
//    WCHAR path[MAX_PATH];
//    HMODULE hModule = GetModuleHandleW(NULL);
//    if (GetModuleFileNameW(hModule, path, MAX_PATH) > 0)
//    {
//        PathRemoveFileSpecW(path);
//        SetCurrentDirectoryW(path);
//    }
//
//    Application::Create(hInstance);
//    {
//        std::shared_ptr<RenderCube> demo = std::make_shared<RenderCube>(L"Learning DirectX 12 - Lesson 2", 1280, 720);
//        retCode = Application::Get().Run(demo);
//    }
//    Application::Destroy();
//
//    atexit(&ReportLiveObjects);
//
//    return retCode;
//}

#include <corecrt_wstdio.h>
#include <d3d12.h>
#include <dxgi.h>
#include <dxgi1_3.h>
#include <wrl/client.h>
#include <dwmapi.h>
#include <dxgi1_4.h>
#include <D3DX12/d3dx12_barriers.h>

#include "Helpers.h"

using namespace Microsoft::WRL;


constexpr int numBackBuffers = 3;
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

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
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
	HWND hWnd = CreateWindowW(wndClassName, L"WindowOne", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 500, 500, nullptr, nullptr, hInstance, nullptr);

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

	MSG msg = {};
	//while (PeekMessage(&msg, nullptr, 0, 0, 1))
	while(GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

void Render()
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
	const FLOAT color[4] = {0.5f, 0.9f, 0.1f, 1};
	g_commandList->ClearRenderTargetView(rtv, color, 0, nullptr);

	{
		// render target -> present
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT);

		g_commandList->ResourceBarrier(1, &barrier);
	}
	
	// execute command list
	ID3D12GraphicsCommandList* rawLists =  g_commandList.Get();
	ID3D12CommandList* lists[] = {rawLists};
	g_commandQueue->ExecuteCommandLists(1, lists);
	g_commandQueue->Signal(g_fence.Get(), fenceValue);
	buffersFenceValue[g_currentBackBuffer] = fenceValue;
	fenceValue++;

	// present
	g_swapChain->Present(1, 0);
	g_currentBackBuffer = g_swapChain->GetCurrentBackBufferIndex();
	UINT waitValue = buffersFenceValue[g_currentBackBuffer];
	if(g_fence->GetCompletedValue() < waitValue)
	{
		HANDLE hEvent = 
		g_fence->SetEventOnCompletion(waitValue, hEvent);
	}
}

LRESULT wWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_PAINT:
		Render();
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}
