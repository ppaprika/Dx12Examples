#include "Application.h"
#include <assert.h>
#include <map>

#include "Helpers.h"
#include "CommandQueue.h"
#include "Game.h"
#include "Window.h"

constexpr wchar_t WINDOW_CLASS_NAME[] = L"DX12RenderWindowClass";

using WindowPtr = std::shared_ptr<Window>;
using WindowMap = std::map< HWND, WindowPtr >;
using WindowNameMap = std::map< std::wstring, WindowPtr >;

static WindowMap gs_Windows;
static WindowNameMap gs_WindowByName;

extern bool g_UseWarp;

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

Application* Application::gs_Application = nullptr;

struct MakeWindow : public Window
{
	MakeWindow(HWND hWnd, const std::wstring& windowName, int clientWidth, int clientHeight, bool vSync)
		: Window(hWnd, windowName, clientWidth, clientHeight, vSync)
	{}
};


void Application::Create(HINSTANCE hInst)
{
	if(!gs_Application)
	{
		gs_Application = new Application(hInst);
	}
}

bool Application::IsTearingSupported() const
{
	return m_TearingSupported;
}

std::shared_ptr<Window> Application::CreateRenderWindow(const std::wstring& windowName, int clientWidth,
	int clientHeight, bool vSync)
{
	// Create Window
	int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);

	RECT windowRect = { 0, 0, clientWidth, clientHeight, };
	::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	int width = windowRect.right - windowRect.left;
	int height = windowRect.bottom - windowRect.top;

	int topLeftCornerX = std::max<int>(0, (screenWidth - width) / 2); 
	int topLeftCornerY = std::max<int>(0, (screenHeight - height) / 2);

	HWND windowHandle = ::CreateWindowExW(
		NULL, 
		WINDOW_CLASS_NAME, 
		windowName.c_str(), 
		WS_OVERLAPPEDWINDOW, 
		topLeftCornerX, 
		topLeftCornerY, 
		width, 
		height, 
		nullptr,
		nullptr,
		m_hInstance,
		nullptr);

	WindowPtr wPtr = std::make_shared<MakeWindow>(windowHandle, windowName, clientWidth, clientHeight, vSync);
	gs_Windows.insert(std::pair<HWND, WindowPtr>(windowHandle, wPtr));
	gs_WindowByName.insert(std::pair<std::wstring, WindowPtr>(windowName, wPtr));

	return wPtr;
}

int Application::Run(std::shared_ptr<Game> pGame)
{
	pGame->Initialize();
	pGame->LoadContent();


}

Application::Application(HINSTANCE hInst)
{
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

#if defined(_DEBUG)
	Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();
#endif

	m_hInstance = hInst;
	m_TearingSupported = CheckTearingSupport();

	// Register Window
	WNDCLASSEXW windowClass = {};

	windowClass.cbSize = sizeof(WNDCLASSEXW);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = &WndProc;
	windowClass.hInstance = m_hInstance;
	windowClass.hIcon = ::LoadIcon(m_hInstance, NULL);
	windowClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	windowClass.lpszMenuName = NULL;
	windowClass.lpszClassName = WINDOW_CLASS_NAME;
	windowClass.hIconSm = ::LoadIcon(m_hInstance, NULL);

	static HRESULT hr = ::RegisterClassExW(&windowClass);
	assert(SUCCEEDED(hr));

	m_dxgiAdapter = GetAdapter(g_UseWarp);
	m_d3d12Device = CreateDevice(m_dxgiAdapter);

	m_DirectCommandQueue = std::make_shared<CommandQueue>(m_d3d12Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_ComputeCommandQueue = std::make_shared<CommandQueue>(m_d3d12Device, D3D12_COMMAND_LIST_TYPE_COMPUTE);
	m_CopyCommandQueue = std::make_shared<CommandQueue>(m_d3d12Device, D3D12_COMMAND_LIST_TYPE_COPY);
}

Microsoft::WRL::ComPtr<IDXGIAdapter4> Application::GetAdapter(bool bUseWarp)
{
	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter1;
	Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter4;

	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

	if(bUseWarp)
	{
		ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&adapter1)));
		ThrowIfFailed(adapter1.As(&adapter4));
	}
	else
	{
		SIZE_T maxDedicatedVideoMemory = 0;
		for(UINT i = 0; dxgiFactory->EnumAdapters1(i, &adapter1) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_ADAPTER_DESC1 adapterDesc1;
			adapter1->GetDesc1(&adapterDesc1);

			if(adapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory && 
				(adapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 && 
				SUCCEEDED(D3D12CreateDevice(adapter1.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)))
			{
				maxDedicatedVideoMemory = adapterDesc1.DedicatedVideoMemory;
				ThrowIfFailed(adapter1.As(&adapter4));
			}
		}
	}

	return adapter4;
}

Microsoft::WRL::ComPtr<ID3D12Device2> Application::CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter)
{
	Microsoft::WRL::ComPtr<ID3D12Device2> device2;
	ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device2)));

#if defined(_DEBUG)
	Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
	if(SUCCEEDED(device2.As(&infoQueue)))
	{
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

		D3D12_MESSAGE_SEVERITY Severities[] = {
			D3D12_MESSAGE_SEVERITY_INFO
		};

		D3D12_MESSAGE_ID DenyIds[] = {
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
		};

		D3D12_INFO_QUEUE_FILTER NewFilter = {};
		NewFilter.DenyList.NumSeverities = _countof(Severities);
		NewFilter.DenyList.pSeverityList = Severities;
		NewFilter.DenyList.NumIDs = _countof(DenyIds);
		NewFilter.DenyList.pIDList = DenyIds;

		ThrowIfFailed(infoQueue->PushStorageFilter(&NewFilter));
	}
#endif

	return device2;
}

bool Application::CheckTearingSupport()
{
	BOOL allowTearing = FALSE;

	Microsoft::WRL::ComPtr<IDXGIFactory4> factory4;
	if(SUCCEEDED(CreateDXGIFactory(IID_PPV_ARGS(&factory4))))
	{
		Microsoft::WRL::ComPtr<IDXGIFactory5> factory5;
		if(SUCCEEDED(factory4.As(&factory5)))
		{
			if(FAILED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
			{
				allowTearing = FALSE;
			}
		}
	}

	return allowTearing == TRUE;
}
