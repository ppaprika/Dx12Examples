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

#include "Helpers.h"

using namespace Microsoft::WRL;


constexpr int numBackBuffers = 3;
constexpr wchar_t wndClassName[] = L"TryIt";
//const wchar_t* wndClassName = L"TryIt";

ComPtr<IDXGIAdapter> g_adapter;
ComPtr<ID3D12Device> g_device;
ComPtr<ID3D12CommandQueue> g_commandQueue;
ComPtr<ID3D12GraphicsCommandList> g_commandList;
ComPtr<IDXGISwapChain> g_swapChain;
ComPtr<ID3D12Resource> g_backBuffers[numBackBuffers];
ComPtr<ID3D12CommandAllocator> g_commandAllocators[numBackBuffers];

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
	UpdateWindow(hWnd);

	// get adapter
	g_adapter = GetAdapter();

	// create device
	g_device = CreateDevice(g_adapter);

	// create command queue
	g_commandQueue = CreateCommandQueue(g_device, D3D12_COMMAND_LIST_TYPE_DIRECT);

	// create swap chain
	g_swapChain = CreateSwapChain(hWnd, g_commandQueue, numBackBuffers);

	// create command allocator
	for(int i = 0; i < numBackBuffers; ++i)
	{
		g_commandAllocators[i] = CreateCommandAllocator(g_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	}

	// create command list
	g_commandList = CreateCommandList(g_commandAllocators[0], g_device, D3D12_COMMAND_LIST_TYPE_DIRECT);






	MSG msg = {};
	//while (PeekMessage(&msg, nullptr, 0, 0, 0))
	while(GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

LRESULT wWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}
