#include "Game.h"

#include <codecvt>
#include <iostream>
#include <ratio>
#include <D3DX12/d3dx12_resource_helpers.h>

#include "Application.h"
#include "DirectCommandList.h"
#include "Helpers.h"
#include "UploadBuffer.h"
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
	if(show_fps_)
	{
		auto now = std::chrono::high_resolution_clock::now();
		delta_time = now - last_tick_;
		last_tick_ = now;
		double fps = 1000 / delta_time.count();
		char buffer[500];
		sprintf_s(buffer, 500, "FPS: %f\n", fps);
		OutputDebugStringA(buffer);
	}
}

int Game::Run(std::shared_ptr<Application> App, CreateWindowParams* Params)
{
	GlobalGame = shared_from_this();

	upload_buffer_ = std::make_shared<UploadBuffer>(App->GetDevice());

	{
		direct_command_list_ = std::make_shared<DirectCommandList>(Application::GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT, Params->numOfBackBuffers);
		Params->command_list = direct_command_list_;
		Params->winProc = Game::StaticWinProc;
		direct_command_list_->CreateTargetWindow(Params);	
	}

	Init();

	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		// Process any messages in the queue.
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	Release();
	return 0;
}

void Game::Release()
{
	direct_command_list_->SingleAndWait();
}

LRESULT Game::StaticWinProc(HWND InHwnd, UINT InMessage, WPARAM InWParam, LPARAM InLParam)
{
	return GlobalGame.lock()->WinProc(InHwnd, InMessage, InWParam, InLParam);
}

LRESULT Game::WinProc(HWND InHwnd, UINT InMessage, WPARAM InWParam, LPARAM InLParam)
{
	return DefWindowProc(InHwnd, InMessage, InWParam, InLParam);
}

void Game::Flush()
{
	direct_command_list_->SingleAndWait();
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
