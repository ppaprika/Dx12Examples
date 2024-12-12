#include "DirectCommandList.h"

#include <intsafe.h>
#include <D3DX12/d3dx12_barriers.h>

#include "Application.h"
#include "Window.h"

#include "Helpers.h"
#include "Primitive.h"


DirectCommandList::DirectCommandList(Microsoft::WRL::ComPtr<ID3D12Device> device, D3D12_COMMAND_LIST_TYPE type, int numOfAllocators)
{
	_device = device;
	_type = type;
	_numOfAllocators = numOfAllocators;

	InitCommandQueue();
	InitAllocators();
	InitCommandList();
	InitFence();
}

void DirectCommandList::CreateTargetWindow(CreateWindowParams* Params)
{
	window = std::make_shared<Window>(*Params);
}

DirectCommandList::~DirectCommandList()
{
	for(auto& allocator : _commandAllocators)
	{
		allocator.Reset();
	}
	_commandList.Reset();
	_commandQueue.Reset();
	_device.Reset();
}

void DirectCommandList::SingleAndWait()
{
	_commandQueue->Signal(_fence.Get(), _fenceValue);
	if(_fence->GetCompletedValue() < _fenceValue)
	{
		HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		_fence->SetEventOnCompletion(_fenceValue, event);
		WaitForSingleObject(event, DWORD_MAX);
	}
	_fenceValue++;
}


void DirectCommandList::DrawSinglePrimitive(Primitive* primitive)
{
	if (window->num_of_back_buffers != _numOfAllocators)
	{
		char buffer[200];
		sprintf_s(buffer, 200, "Error: Window's back buffer num %d is not equal to CommandList's allocator num %d !\n", window->num_of_back_buffers, _numOfAllocators);
		OutputDebugStringA(buffer);
		return;
	}

	CD3DX12_VIEWPORT& viewport = window->viewport;
	D3D12_RECT& d3d12Rect = window->d3d12_rect;
	UINT& currentBackBuffer = window->current_back_buffer;
	ComPtr<ID3D12DescriptorHeap> descriptorHeap = window->rtv_heap;
	ComPtr<IDXGISwapChain3> swapChain = window->swap_chain;

	auto dsv = window->GetDSVHandle();
	ComPtr<ID3D12Resource> backBuffer = window->GetCurrentBackBuffer();
	auto rtv = window->GetRTVHandle();

	ComPtr<ID3D12CommandAllocator> allocator = _commandAllocators[currentBackBuffer];

	_commandList->Reset(allocator.Get(), nullptr);

	// clear render target
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		_commandList->ResourceBarrier(1, &barrier);

		// clear color
		FLOAT color[4] = { 0.6f, 0.2f, 0.2f, 1 };
		_commandList->ClearRenderTargetView(rtv, color, 0, nullptr);

		_commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	}
	_commandList->RSSetViewports(1, &viewport);
	_commandList->RSSetScissorRects(1, &d3d12Rect);
	_commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

	primitive->PrepareForDrawing(_commandList);
	primitive->SetRootParams(_commandList);

	_commandList->DrawIndexedInstanced(primitive->GetIndexCount(), 1, 0, 0, 0);
	// present
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		_commandList->ResourceBarrier(1, &barrier);
		_commandList->Close();

		ID3D12CommandList* lists[] = { _commandList.Get() };
		_commandQueue->ExecuteCommandLists(1, lists);
		_commandQueue->Signal(_fence.Get(), _fenceValue);
		_waitingValue[currentBackBuffer] = _fenceValue;
		_fenceValue++;

		swapChain->Present(1, 0);

		currentBackBuffer = swapChain->GetCurrentBackBufferIndex();
		UINT waitValue = _waitingValue[currentBackBuffer];
		if (_fence->GetCompletedValue() < waitValue)
		{
			HANDLE hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
			_fence->SetEventOnCompletion(waitValue, hEvent);
			WaitForSingleObject(hEvent, INFINITE);
		}
	}
}

void DirectCommandList::Reset()
{
	auto dsv = window->GetDSVHandle();
	ComPtr<ID3D12Resource> backBuffer = window->GetCurrentBackBuffer();
	auto rtv = window->GetRTVHandle();

	ComPtr<ID3D12CommandAllocator> allocator = _commandAllocators[window->current_back_buffer];

	_commandList->Reset(allocator.Get(), nullptr);

	// clear render target
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		_commandList->ResourceBarrier(1, &barrier);

		// clear color
		FLOAT color[4] = { 0.6f, 0.2f, 0.2f, 1 };
		_commandList->ClearRenderTargetView(rtv, color, 0, nullptr);

		_commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	}
	_commandList->RSSetViewports(1, &window->viewport);
	_commandList->RSSetScissorRects(1, &window->d3d12_rect);
	_commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
}

void DirectCommandList::Draw(Primitive* primitive)
{
	primitive->PrepareForDrawing(_commandList);
	primitive->Draw(_commandList);
}

void DirectCommandList::Present()
{
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(window->GetCurrentBackBuffer().Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	_commandList->ResourceBarrier(1, &barrier);
	_commandList->Close();

	ID3D12CommandList* lists[] = { _commandList.Get() };
	_commandQueue->ExecuteCommandLists(1, lists);
	_commandQueue->Signal(_fence.Get(), _fenceValue);
	_waitingValue[window->current_back_buffer] = _fenceValue;
	_fenceValue++;

	window->swap_chain->Present(1, 0);

	window->current_back_buffer = window->swap_chain->GetCurrentBackBufferIndex();
	UINT waitValue = _waitingValue[window->current_back_buffer];
	if (_fence->GetCompletedValue() < waitValue)
	{
		HANDLE hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
		_fence->SetEventOnCompletion(waitValue, hEvent);
		WaitForSingleObject(hEvent, INFINITE);
	}
}

int DirectCommandList::GetTargetWindowWidth() const
{
	return window->GetWidth();
}

int DirectCommandList::GetTargetWindowHeight() const
{
	return window->GetHeight();
}

void DirectCommandList::InitCommandQueue()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = _type;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT;
	queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
	queueDesc.NodeMask = 0;

	ThrowIfFailed(_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_commandQueue)));
}

void DirectCommandList::InitAllocators()
{
	for(int i = 0; i < _numOfAllocators; ++i)
	{
		ComPtr<ID3D12CommandAllocator> tempAllocator;
		ThrowIfFailed(_device->CreateCommandAllocator(_type, IID_PPV_ARGS(&tempAllocator)));
		_commandAllocators.push_back(tempAllocator);
	}
}

void DirectCommandList::InitCommandList()
{
	ComPtr<ID3D12GraphicsCommandList> tempList;
	ThrowIfFailed(_device->CreateCommandList(0, _type, _commandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&tempList)));
	tempList.As(&_commandList);
}

void DirectCommandList::InitFence()
{
	ThrowIfFailed(_device->CreateFence(_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)));
	for(int i = 0; i < _numOfAllocators; ++i)
	{
		_waitingValue.push_back(_fenceValue);
	}
}
