#include "CommandList.h"

#include <intsafe.h>
#include <D3DX12/d3dx12_barriers.h>

#include "Window.h"

#include "Helpers.h"


CommandList::CommandList(Microsoft::WRL::ComPtr<ID3D12Device> device, D3D12_COMMAND_LIST_TYPE type, int numOfAllocators)
{
	_device = device;
	_type = type;
	_numOfAllocators = numOfAllocators;

	InitCommandQueue();
	InitAllocators();
	InitCommandList();
}

CommandList::~CommandList()
{
	for(auto& allocator : _commandAllocators)
	{
		allocator.Reset();
	}
	_commandList.Reset();
	_commandQueue.Reset();
	_device.Reset();
}

void CommandList::Execute()
{
	_commandList->Close();
	ID3D12CommandList* lists[] = { _commandList.Get() };
	_commandQueue->ExecuteCommandLists(1, lists);
}

void CommandList::SingleAndWait(ComPtr<ID3D12Fence> fence, UINT fenceValue)
{
	_commandQueue->Signal(fence.Get(), fenceValue);
	if(fence->GetCompletedValue() < fenceValue)
	{
		HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		fence->SetEventOnCompletion(fenceValue, event);
		WaitForSingleObject(event, DWORD_MAX);
	}
}

void CommandList::DrawToWindow(std::shared_ptr<class Window> Window, DrawWindowParams& Params)
{
	CD3DX12_VIEWPORT& viewport = Window->_viewport;
	D3D12_RECT& d3d12Rect = Window->_d3d12Rect;

	std::vector<ComPtr<ID3D12Resource>>& backBuffers = Window->_backBuffers;
	UINT& currentBackBuffer = Window->_currentBackBuffer;
	ComPtr<ID3D12DescriptorHeap> descriptorHeap = Window->_descriptorHeap;
	SIZE_T heapSize = Window->_heapSize;
	ComPtr<IDXGISwapChain3> swapChain = Window->_swapChain;

	UINT& fenceValue = Window->_fenceValue;
	ComPtr<ID3D12Fence> fence = Window->_fence;
	std::vector<UINT>& buffersFenceValue = Window->_waitingValue;


	ComPtr<ID3D12Resource> backBuffer = backBuffers[currentBackBuffer];
	ComPtr<ID3D12CommandAllocator> allocator = _commandAllocators[currentBackBuffer];
	auto rtv = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	rtv.ptr += currentBackBuffer * heapSize;
	auto dsv = Window->_dsvHeap->GetCPUDescriptorHandleForHeapStart();

	allocator->Reset();
	_commandList->Reset(allocator.Get(), nullptr);

	// clear render target
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		_commandList->ResourceBarrier(1, &barrier);

		// clear color
		FLOAT color[4] = { 0.f, 0.f, 0.f, 1 };
		_commandList->ClearRenderTargetView(rtv, color, 0, nullptr);

		_commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	}

	_commandList->SetPipelineState(Params.PSO.Get());
	_commandList->SetGraphicsRootSignature(Params.RootSignature.Get());

	_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_commandList->IASetIndexBuffer(&Params.IndexBufferView);
	_commandList->IASetVertexBuffers(0, 1, &Params.VertexBufferView);

	_commandList->RSSetViewports(1, &viewport);
	_commandList->RSSetScissorRects(1, &d3d12Rect);

	_commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

	Params.SetRootConstant(_commandList);

	_commandList->DrawIndexedInstanced(Params.DrawNum, 1, 0, 0, 0);
	// present
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		_commandList->ResourceBarrier(1, &barrier);
		_commandList->Close();

		ID3D12CommandList* lists[] = { _commandList.Get() };
		_commandQueue->ExecuteCommandLists(1, lists);
		_commandQueue->Signal(fence.Get(), fenceValue);
		buffersFenceValue[currentBackBuffer] = fenceValue;
		fenceValue++;

		swapChain->Present(1, 0);

		currentBackBuffer = swapChain->GetCurrentBackBufferIndex();
		UINT waitValue = buffersFenceValue[currentBackBuffer];
		if (fence->GetCompletedValue() < waitValue)
		{
			HANDLE hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
			fence->SetEventOnCompletion(waitValue, hEvent);
			WaitForSingleObject(hEvent, INFINITE);
		}
	}
}

void CommandList::InitCommandQueue()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = _type;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT;
	queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
	queueDesc.NodeMask = 0;

	ThrowIfFailed(_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_commandQueue)));
}

void CommandList::InitAllocators()
{
	for(int i = 0; i < _numOfAllocators; ++i)
	{
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> tempAllocator;
		ThrowIfFailed(_device->CreateCommandAllocator(_type, IID_PPV_ARGS(&tempAllocator)));
		_commandAllocators.push_back(tempAllocator);
	}
}

void CommandList::InitCommandList()
{
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> tempList;
	ThrowIfFailed(_device->CreateCommandList(0, _type, _commandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&tempList)));
	tempList.As(&_commandList);
}
