#include "Window.h"

#include "Application.h"
#include "Helpers.h"
#include "DirectCommandList.h"

Window::Window(const CreateWindowParams& Params)
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

	width_ = Params.nWidth;
	height_ = Params.nHeight;
	num_of_back_buffers = Params.numOfBackBuffers;
	current_back_buffer = 0;

	window_ = CreateWindowW(Params.wndClassName, Params.wndName, Params.dwStyle, Params.x, Params.y, Params.nWidth, Params.nHeight, nullptr, nullptr, Params.hInstance, nullptr);
	if(window_ == nullptr)
	{
		DWORD error = GetLastError();
		wchar_t errorMsg[256];
		FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error, 0, errorMsg, 256, nullptr);
		MessageBox(nullptr, errorMsg, L"Error", MB_ICONERROR);
		ExitProcess(1);
	}

	ShowWindow(window_, Params.nCmdShow);

	CreateSwapChain(window_, Params.command_list->_commandQueue, Params.numOfBackBuffers).As(&swap_chain);
	rtv_heap = CreateDescriptorHeap(Application::GetDevice(), Params.numOfBackBuffers, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	UpdateRenderTarget(Application::GetDevice(), swap_chain, back_buffers, Params.numOfBackBuffers, rtv_heap, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	owner_ = Params.command_list;

	InitViewportAndRect();
	InitDepth();
}

Window::~Window()
{
}

void Window::InitViewportAndRect()
{
	viewport = CD3DX12_VIEWPORT(0., 0., width_, height_, D3D12_MIN_DEPTH, D3D12_MAX_DEPTH);
	d3d12_rect = CD3DX12_RECT(0, 0, width_, height_);
}

void Window::InitDepth()
{
	// create dsv heap
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.NumDescriptors = 1;
	ThrowIfFailed(Application::GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&dsv_heap)));

	ResizeDepthBuffer();
}

void Window::ResizeDepthBuffer()
{
	D3D12_CLEAR_VALUE optimizedClearValue = {};
	optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	optimizedClearValue.DepthStencil = { 1.0f, 0 };

	ComPtr<ID3D12Device> device = Application::GetDevice();

	CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width_, height_, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	ThrowIfFailed(device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&optimizedClearValue,
		IID_PPV_ARGS(&depth_buffer)
	));

	D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
	desc.Format = DXGI_FORMAT_D32_FLOAT;
	desc.Flags = D3D12_DSV_FLAG_NONE;
	desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipSlice = 0;

	auto dsv = dsv_heap->GetCPUDescriptorHandleForHeapStart();
	device->CreateDepthStencilView(depth_buffer.Get(), &desc, dsv);
}

void Window::UpdateSize(int width, int height)
{
	width_ = width;
	height_ = height;

	if(auto CommandList = owner_.lock())
	{
		CommandList->SingleAndWait();
	}
	else
	{
		return;
	}
	InitViewportAndRect();
	ResizeDepthBuffer();

	for(UINT i = 0; i < num_of_back_buffers; ++i)
	{
		back_buffers[i].Reset();
	}
	swap_chain->ResizeBuffers(num_of_back_buffers, width_, height_, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
	UpdateRenderTarget(Application::GetDevice(), swap_chain, back_buffers, num_of_back_buffers, rtv_heap, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	current_back_buffer = swap_chain->GetCurrentBackBufferIndex();
}

ComPtr<ID3D12Resource> Window::GetCurrentBackBuffer()
{
	return back_buffers[current_back_buffer];
}

D3D12_CPU_DESCRIPTOR_HANDLE Window::GetDSVHandle()
{
	return dsv_heap->GetCPUDescriptorHandleForHeapStart();
}

D3D12_CPU_DESCRIPTOR_HANDLE Window::GetRTVHandle()
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtv_heap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += (current_back_buffer * Application::GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
	return rtvHandle;
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
	D3D12_DESCRIPTOR_HEAP_TYPE type)
{
	D3D12_RENDER_TARGET_VIEW_DESC rtDesc = {};
	rtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	rtDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	D3D12_CPU_DESCRIPTOR_HANDLE rtHandle = heap->GetCPUDescriptorHandleForHeapStart();

	SIZE_T heapSize = Application::GetDescriptorSize(type);

	for (UINT i = 0; i < bufferNum; ++i)
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

