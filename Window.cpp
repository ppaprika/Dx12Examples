#include "Window.h"

#include <D3DX12/d3dx12_root_signature.h>

#include "Application.h"
#include "CommandQueue.h"
#include "Helpers.h"

HWND Window::GetWindowHandle() const
{
	return m_hWnd;
}

void Window::Destroy()
{
}

const std::wstring& Window::GetWindowName() const
{
	return m_WindowName;
}

void Window::Show()
{
	::ShowWindow(m_hWnd, SW_SHOW);
}

Window::Window(HWND hWnd, const std::wstring& windowName, int clientWidth, int clientHeight, bool vSync)
{
	m_hWnd = hWnd;
	m_WindowName = windowName;
	m_ClientWidth = clientWidth;
	m_ClientHeight = clientHeight;
	m_VSync = vSync;

	m_FrameCounter = 0;

	Application& app = Application::Get();
	m_IsTearingSupported = app.IsTearingSupported();
	CreateSwapChain();

	// create rtv on swap chain's back buffers, and store back buffers' reference
	UpdateRenderTargetViews();
}

void Window::RegisterCallbacks(std::shared_ptr<Game> pGame)
{
	m_pGame = pGame;
}

void Window::OnUpdate(UpdateEventArgs& e)
{
}

void Window::OnRender(RenderEventArgs& e)
{
}

void Window::OnMouseMoved(MouseMotionEventArgs& e)
{
}

Microsoft::WRL::ComPtr<IDXGISwapChain4> Window::CreateSwapChain()
{
	Application& app = Application::Get();

	Microsoft::WRL::ComPtr<IDXGIFactory4> factory4;
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	// create swap chain
	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&factory4)));
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
	swapChainDesc.Width = m_ClientWidth;
	swapChainDesc.Height = m_ClientHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc = { 1, 0 };
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = BufferCount;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags = m_IsTearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;


	Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(factory4->CreateSwapChainForHwnd(
		app.GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT)->GetD3D12CommandQueue().Get(),
		m_hWnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain1
	));

	ThrowIfFailed(swapChain1.As(&m_dxgiSwapChain));

	m_CurrentBackBufferIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();

	m_RTVDescriptorSize = app.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// create descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.NumDescriptors = BufferCount;

	ThrowIfFailed(app.GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_d3d12RTVDescriptorHeap)));

	return m_dxgiSwapChain;
}

void Window::UpdateRenderTargetViews()
{
	Application& app = Application::Get();

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_d3d12RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	for(int i = 0; i < BufferCount; ++i)
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer;
		ThrowIfFailed(m_dxgiSwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

		app.GetDevice()->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);
		m_d3d12BackBuffers[i] = backBuffer;
		rtvHandle.Offset(m_RTVDescriptorSize);
	}
}


