#include "Window.h"

#include "Helpers.h"

Window::Window(CreateWindowParams& Params)
{
	WNDCLASSW windowClass = {};
	windowClass.style = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW;
	windowClass.lpfnWndProc = Params.winProc;
	windowClass.hInstance = Params.hInstance;
	windowClass.lpszClassName = Params.wndClassName;

	if(!RegisterClassW(&windowClass))
	{
		MessageBox(nullptr, L"Failed to register window class!", L"Error", MB_ICONERROR);
		ExitProcess(1);
	}

	_window = CreateWindowW(Params.wndClassName, Params.wndName, Params.dwStyle, Params.x, Params.y, Params.nWidth, Params.nHeight, nullptr, nullptr, Params.hInstance, nullptr);
	if(_window == nullptr)
	{
		DWORD error = GetLastError();
		wchar_t errorMsg[256];
		swprintf_s(errorMsg, L"CreateWindowW failed with error %lu", error);
		MessageBox(nullptr, errorMsg, L"Error", MB_ICONERROR);
		ExitProcess(1);
	}

	ShowWindow(_window, Params.nCmdShow);

	CreateSwapChain(_window, Params.queue, Params.numOfBackBuffers).As(&_swapChain);
	_descriptorHeap = CreateDescriptorHeap(Params.device, Params.numOfBackBuffers, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	UpdateRenderTarget(Params.device, _swapChain, _backBuffers, Params.numOfBackBuffers, _descriptorHeap, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, _heapSize);
}

Window::~Window()
{
}

ComPtr<IDXGISwapChain> Window::CreateSwapChain(HWND window, ComPtr<ID3D12CommandQueue> queue, int numOfBackBuffers)
{
	DXGI_MODE_DESC modeDesc = {};
	modeDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	DXGI_SWAP_CHAIN_DESC swapChaindesc = {};
	swapChaindesc.BufferDesc = modeDesc;
	swapChaindesc.BufferCount = numOfBackBuffers;
	swapChaindesc.OutputWindow = window;
	swapChaindesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChaindesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChaindesc.SampleDesc = { 1, 0 };
	swapChaindesc.Windowed = true;
	swapChaindesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	ComPtr<IDXGIFactory2> factory;
	ComPtr<IDXGISwapChain> value;
	ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory)));
	ThrowIfFailed(factory->CreateSwapChain(queue.Get(), &swapChaindesc, &value));

	return value;
}

bool Window::UpdateRenderTarget(ComPtr<ID3D12Device> device, ComPtr<IDXGISwapChain> swapChain,
	std::vector<ComPtr<ID3D12Resource>>& resources, UINT bufferNum, ComPtr<ID3D12DescriptorHeap> heap,
	D3D12_DESCRIPTOR_HEAP_TYPE type, SIZE_T& heapSize)
{
	D3D12_RENDER_TARGET_VIEW_DESC rtDesc = {};
	rtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	rtDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	D3D12_CPU_DESCRIPTOR_HANDLE rtHandle = heap->GetCPUDescriptorHandleForHeapStart();

	heapSize = device->GetDescriptorHandleIncrementSize(type);

	for (int i = 0; i < bufferNum; ++i)
	{
		ComPtr<ID3D12Resource> buffer;
		swapChain->GetBuffer(i, IID_PPV_ARGS(&buffer));
		device->CreateRenderTargetView(buffer.Get(), &rtDesc, rtHandle);
		if(resources.size() <= i)
		{
			resources.push_back(buffer);
		}
		else
		{
			resources[i] = buffer;
		}

		rtHandle.ptr += heapSize;
	}

	return true;
}

ComPtr<ID3D12DescriptorHeap> Window::CreateDescriptorHeap(ComPtr<ID3D12Device> device, UINT num,
	D3D12_DESCRIPTOR_HEAP_TYPE type)
{
	ComPtr<ID3D12DescriptorHeap> value;
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = num;
	heapDesc.Type = type;

	ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&value)));

	return value;
}
