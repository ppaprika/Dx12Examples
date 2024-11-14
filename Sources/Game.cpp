#include "Game.h"

#include <codecvt>
#include <iostream>
#include <ratio>
#include <D3DX12/d3dx12_resource_helpers.h>

#include "Application.h"
#include "Helpers.h"
#include "Window.h"

std::weak_ptr<Game> Game::GlobalGame;

Game::Game()
{
}

Game::~Game()
{
}

void Game::Update()
{
	if(_showFps)
	{
		auto now = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> elapsed = now - _lastTick;
		_lastTick = now;
		double fps = 1000 / elapsed.count();
		char buffer[500];
		sprintf_s(buffer, 500, "FPS: %f\n", fps);
		OutputDebugStringA(buffer);
	}
}

int Game::Run(std::shared_ptr<Application> App, CreateWindowParams* Params)
{
	_app = App;
	GlobalGame = shared_from_this();

	if(Params)
	{
		Params->winProc = Game::StaticWinProc;
		_window = std::make_shared<Window>(shared_from_this(), *Params);
		_window->InitWindow();
	}

	Init();

	MSG msg = {};
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

LRESULT Game::StaticWinProc(HWND InHwnd, UINT InMessage, WPARAM InWParam, LPARAM InLParam)
{
	return GlobalGame.lock()->WinProc(InHwnd, InMessage, InWParam, InLParam);
}

LRESULT Game::WinProc(HWND InHwnd, UINT InMessage, WPARAM InWParam, LPARAM InLParam)
{
	_hwnd = InHwnd;
	_message = InMessage;
	_wParam = InWParam;
	_lParam = InLParam;

	//switch (InMessage)
	//{
	//case WM_PAINT:
	//	Update();
	//	Render();
	//	return 0;
	//default:
	//	return DefWindowProc(InHwnd, InMessage, InWParam, InLParam);
	//}

	return DefWindowProc(InHwnd, InMessage, InWParam, InLParam);
}

ComPtr<ID3D12Device> Game::GetDevice()
{
	if(!_app.expired())
	{
		return _app.lock()->GetDevice();
	}
	return nullptr;
}

void Game::UpdateBufferResource(ComPtr<ID3D12GraphicsCommandList2> commandList, ID3D12Resource** pDestinationResource,
	ID3D12Resource** pIntermediateResource, size_t numElements, size_t elementSize, const void* bufferData,
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

	if (bufferData)
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
