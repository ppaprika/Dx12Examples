#include "Window.h"

#include "Helpers.h"
#include "CommandList.h"

Window::Window(std::shared_ptr<Game> Owner, const CreateWindowParams& Params)
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

	_width = Params.nWidth;
	_height = Params.nHeight;
	_numOfBackBuffers = Params.numOfBackBuffers;
	_heapSize = Owner->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	_currentBackBuffer = 0;

	_window = CreateWindowW(Params.wndClassName, Params.wndName, Params.dwStyle, Params.x, Params.y, Params.nWidth, Params.nHeight, nullptr, nullptr, Params.hInstance, nullptr);
	if(_window == nullptr)
	{
		DWORD error = GetLastError();
		wchar_t errorMsg[256];
		FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error, 0, errorMsg, 256, nullptr);
		MessageBox(nullptr, errorMsg, L"Error", MB_ICONERROR);
		ExitProcess(1);
	}

	ShowWindow(_window, Params.nCmdShow);

	_commandList = std::make_shared<CommandList>(Owner->GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT, Params.numOfBackBuffers);

	CreateSwapChain(_window, _commandList->_commandQueue, Params.numOfBackBuffers).As(&_swapChain);
	_descriptorHeap = CreateDescriptorHeap(Owner->GetDevice(), Params.numOfBackBuffers, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	UpdateRenderTarget(Owner->GetDevice(), _swapChain, _backBuffers, Params.numOfBackBuffers, _descriptorHeap, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, _heapSize);
	_fence = CreateFence(Owner->GetDevice(), _fenceValue);
	for(int i = 0; i < Params.numOfBackBuffers; ++i)
	{
		_waitingValue.push_back(_fenceValue);
	}
	_owner = Owner;
}

Window::~Window()
{
}

void Window::InitWindow()
{
	InitViewportAndRect();
	InitDepth();
}

void Window::InitViewportAndRect()
{
	_viewport = CD3DX12_VIEWPORT(0., 0., _width, _height, D3D12_MIN_DEPTH, D3D12_MAX_DEPTH);
	_d3d12Rect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
}

void Window::InitDepth()
{
	// create dev heap
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.NumDescriptors = 1;
	ThrowIfFailed(_owner.lock()->GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_dsvHeap)));

	ResizeDepthBuffer();
}

void Window::ResizeDepthBuffer()
{
	D3D12_CLEAR_VALUE optimizedClearValue = {};
	optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	optimizedClearValue.DepthStencil = { 1.0f, 0 };

	ComPtr<ID3D12Device> device = _owner.lock()->GetDevice();

	CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, _width, _height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	ThrowIfFailed(device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&optimizedClearValue,
		IID_PPV_ARGS(&_depthBuffer)
	));

	D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
	desc.Format = DXGI_FORMAT_D32_FLOAT;
	desc.Flags = D3D12_DSV_FLAG_NONE;
	desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipSlice = 0;

	auto dsv = _dsvHeap->GetCPUDescriptorHandleForHeapStart();
	device->CreateDepthStencilView(_depthBuffer.Get(), &desc, dsv);
}

void Window::UpdateSize(int width, int height)
{
	_width = width;
	_height = height;

	_viewport = CD3DX12_VIEWPORT(0., 0., _width, _height, D3D12_MIN_DEPTH, D3D12_MAX_DEPTH);
	Flush();

	ResizeDepthBuffer();

	for(int i = 0; i < _numOfBackBuffers; ++i)
	{
		_backBuffers[i].Reset();
	}
	_swapChain->ResizeBuffers(_numOfBackBuffers, _width, _height, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
	UpdateRenderTarget(_owner.lock()->GetDevice(), _swapChain, _backBuffers, _numOfBackBuffers, _descriptorHeap, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, _heapSize);
	_currentBackBuffer = _swapChain->GetCurrentBackBufferIndex();
}

void Window::Flush()
{
	_commandList->SingleAndWait(_fence, _fenceValue);
	_fenceValue++;
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

ComPtr<ID3D12Fence> Window::CreateFence(ComPtr<ID3D12Device> device, UINT fenceValue)
{
	ComPtr<ID3D12Fence> value;
	ThrowIfFailed(device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&value)));

	return value;
}
