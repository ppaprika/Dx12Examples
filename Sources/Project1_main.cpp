

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
#include <windowsx.h>


#include "Application.h"
#include "Helpers.h"
#include "CommandList.h"
#include "Window.h"
#include "Game.h"
#include "RenderCube.h"

using namespace Microsoft::WRL;
using namespace DirectX;



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

	CreateWindowParams Params = {};
	Params.hInstance = hInstance;
	Params.wndClassName = L"TryIt";
	Params.wndName = L"WindowOne";
	Params.dwStyle = WS_OVERLAPPEDWINDOW;
	Params.x = 100;
	Params.y = 100;
	Params.nWidth = 600;
	Params.nHeight = 600;
	Params.nCmdShow = nCmdShow;
	Params.numOfBackBuffers = 3;

	std::shared_ptr<Application> App = Application::GetApplication();
	std::shared_ptr<RenderCube> PureGame = std::make_shared<RenderCube>();
	App->Run(PureGame, &Params);

	MSG msg = {};
	while(GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}