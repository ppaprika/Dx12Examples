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

//static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

// Remove a window from our window lists.
static void RemoveWindow(HWND hWnd)
{
	WindowMap::iterator windowIter = gs_Windows.find(hWnd);
	if (windowIter != gs_Windows.end())
	{
		WindowPtr pWindow = windowIter->second;
		gs_WindowByName.erase(pWindow->GetWindowName());
		gs_Windows.erase(windowIter);
	}
}

// Convert the message ID into a MouseButton ID
MouseButtonEventArgs::MouseButton DecodeMouseButton(UINT messageID)
{
	MouseButtonEventArgs::MouseButton mouseButton = MouseButtonEventArgs::None;
	switch (messageID)
	{
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	{
		mouseButton = MouseButtonEventArgs::Left;
	}
	break;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
	{
		mouseButton = MouseButtonEventArgs::Right;
	}
	break;
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
	{
		mouseButton = MouseButtonEventArgs::Middel;
	}
	break;
	}

	return mouseButton;
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	WindowPtr pWindow;
	{
		WindowMap::iterator iter = gs_Windows.find(hwnd);
		if(iter != gs_Windows.end())
		{
			pWindow = iter->second;
		}
	}

	if(pWindow)
	{
		switch (message)
		{
		case WM_PAINT:
			{
			UpdateEventArgs updateEventArgs(0.0f, 0.0f);
			pWindow->OnUpdate(updateEventArgs);
			RenderEventArgs renderEventArgs(0.0f, 0.0f);
			pWindow->OnRender(renderEventArgs);
			}
			break;
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			MSG charMsg;
			// Get the Unicode character (UTF-16)
			unsigned int c = 0;
			// For printable characters, the next message will be WM_CHAR.
			// This message contains the character code we need to send the KeyPressed event.
			// Inspired by the SDL 1.2 implementation.
			if (PeekMessage(&charMsg, hwnd, 0, 0, PM_NOREMOVE) && charMsg.message == WM_CHAR)
			{
				GetMessage(&charMsg, hwnd, 0, 0);
				c = static_cast<unsigned int>(charMsg.wParam);
			}
			bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
			bool control = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
			bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
			KeyCode::Key key = (KeyCode::Key)wParam;
			unsigned int scanCode = (lParam & 0x00FF0000) >> 16;
			KeyEventArgs keyEventArgs(key, c, KeyEventArgs::Pressed, shift, control, alt);
			pWindow->OnKeyPressed(keyEventArgs);
		}
		break;
		case WM_SYSKEYUP:
		case WM_KEYUP:
		{
			bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
			bool control = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
			bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
			KeyCode::Key key = (KeyCode::Key)wParam;
			unsigned int c = 0;
			unsigned int scanCode = (lParam & 0x00FF0000) >> 16;

			// Determine which key was released by converting the key code and the scan code
			// to a printable character (if possible).
			// Inspired by the SDL 1.2 implementation.
			unsigned char keyboardState[256];
			GetKeyboardState(keyboardState);
			wchar_t translatedCharacters[4];
			if (int result = ToUnicodeEx(static_cast<UINT>(wParam), scanCode, keyboardState, translatedCharacters, 4, 0, NULL) > 0)
			{
				c = translatedCharacters[0];
			}

			KeyEventArgs keyEventArgs(key, c, KeyEventArgs::Released, shift, control, alt);
			pWindow->OnKeyReleased(keyEventArgs);
		}
		break;
		// The default window procedure will play a system notification sound 
		// when pressing the Alt+Enter keyboard combination if this message is 
		// not handled.
		case WM_SYSCHAR:
			break;
		case WM_MOUSEMOVE:
		{
			bool lButton = (wParam & MK_LBUTTON) != 0;
			bool rButton = (wParam & MK_RBUTTON) != 0;
			bool mButton = (wParam & MK_MBUTTON) != 0;
			bool shift = (wParam & MK_SHIFT) != 0;
			bool control = (wParam & MK_CONTROL) != 0;

			int x = ((int)(short)LOWORD(lParam));
			int y = ((int)(short)HIWORD(lParam));

			MouseMotionEventArgs mouseMotionEventArgs(lButton, mButton, rButton, control, shift, x, y);
			pWindow->OnMouseMoved(mouseMotionEventArgs);
		}
		break;
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		{
			bool lButton = (wParam & MK_LBUTTON) != 0;
			bool rButton = (wParam & MK_RBUTTON) != 0;
			bool mButton = (wParam & MK_MBUTTON) != 0;
			bool shift = (wParam & MK_SHIFT) != 0;
			bool control = (wParam & MK_CONTROL) != 0;

			int x = ((int)(short)LOWORD(lParam));
			int y = ((int)(short)HIWORD(lParam));

			MouseButtonEventArgs mouseButtonEventArgs(DecodeMouseButton(message), MouseButtonEventArgs::Pressed, lButton, mButton, rButton, control, shift, x, y);
			pWindow->OnMouseButtonPressed(mouseButtonEventArgs);
		}
		break;
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		{
			bool lButton = (wParam & MK_LBUTTON) != 0;
			bool rButton = (wParam & MK_RBUTTON) != 0;
			bool mButton = (wParam & MK_MBUTTON) != 0;
			bool shift = (wParam & MK_SHIFT) != 0;
			bool control = (wParam & MK_CONTROL) != 0;

			int x = ((int)(short)LOWORD(lParam));
			int y = ((int)(short)HIWORD(lParam));

			MouseButtonEventArgs mouseButtonEventArgs(DecodeMouseButton(message), MouseButtonEventArgs::Released, lButton, mButton, rButton, control, shift, x, y);
			pWindow->OnMouseButtonReleased(mouseButtonEventArgs);
		}
		break;
		case WM_MOUSEWHEEL:
		{
			// The distance the mouse wheel is rotated.
			// A positive value indicates the wheel was rotated to the right.
			// A negative value indicates the wheel was rotated to the left.
			float zDelta = ((int)(short)HIWORD(wParam)) / (float)WHEEL_DELTA;
			short keyStates = (short)LOWORD(wParam);

			bool lButton = (keyStates & MK_LBUTTON) != 0;
			bool rButton = (keyStates & MK_RBUTTON) != 0;
			bool mButton = (keyStates & MK_MBUTTON) != 0;
			bool shift = (keyStates & MK_SHIFT) != 0;
			bool control = (keyStates & MK_CONTROL) != 0;

			int x = ((int)(short)LOWORD(lParam));
			int y = ((int)(short)HIWORD(lParam));

			// Convert the screen coordinates to client coordinates.
			POINT clientToScreenPoint;
			clientToScreenPoint.x = x;
			clientToScreenPoint.y = y;
			ScreenToClient(hwnd, &clientToScreenPoint);

			MouseWheelEventArgs mouseWheelEventArgs(zDelta, lButton, mButton, rButton, control, shift, (int)clientToScreenPoint.x, (int)clientToScreenPoint.y);
			pWindow->OnMouseWheel(mouseWheelEventArgs);
		}
		break;
		case WM_SIZE:
		{
			int width = ((int)(short)LOWORD(lParam));
			int height = ((int)(short)HIWORD(lParam));

			ResizeEventArgs resizeEventArgs(width, height);
			pWindow->OnResize(resizeEventArgs);
		}
		break;
		case WM_DESTROY:
		{
			// If a window is being destroyed, remove it from the 
			// window maps.
			RemoveWindow(hwnd);

			if (gs_Windows.empty())
			{
				// If there are no more windows, quit the application.
				PostQuitMessage(0);
			}
		}
		break;
		default:
			return DefWindowProcW(hwnd, message, wParam, lParam);
		}
	}

}

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
