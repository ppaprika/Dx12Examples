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


void DirectCommandList::DrawToWindow(std::shared_ptr<class Window> window, DrawWindowParams& params)
{
	if(window->_numOfBackBuffers != _numOfAllocators)
	{
		char buffer[200];
		sprintf_s(buffer,200, "Error: Window's back buffer num %d is not equal to CommandList's allocator num %d !\n", window->_numOfBackBuffers, _numOfAllocators);
		OutputDebugStringA(buffer);
		return;
	}

	CD3DX12_VIEWPORT& viewport = window->_viewport;
	D3D12_RECT& d3d12Rect = window->_d3d12Rect;
	std::vector<ComPtr<ID3D12Resource>>& backBuffers = window->_backBuffers;
	UINT& currentBackBuffer = window->_currentBackBuffer;
	ComPtr<ID3D12DescriptorHeap> descriptorHeap = window->rtv_heap;
	ComPtr<IDXGISwapChain3> swapChain = window->_swapChain;
	auto dsv = window->dsv_heap->GetCPUDescriptorHandleForHeapStart();

	SIZE_T heapSize = Application::GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	ComPtr<ID3D12Resource> backBuffer = backBuffers[currentBackBuffer];
	ComPtr<ID3D12CommandAllocator> allocator = _commandAllocators[currentBackBuffer];
	auto rtv = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	rtv.ptr += currentBackBuffer * heapSize;

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

	
	_commandList->SetPipelineState(params.PSO.Get());
	_commandList->SetGraphicsRootSignature(params.RootSignature.Get());
	_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_commandList->IASetIndexBuffer(params.IndexBufferView);
	_commandList->IASetVertexBuffers(0, 1, params.VertexBufferView);
	params.SetRootConstant(_commandList);

	_commandList->RSSetViewports(1, &viewport);
	_commandList->RSSetScissorRects(1, &d3d12Rect);

	_commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

	_commandList->DrawIndexedInstanced(params.DrawNum, 1, 0, 0, 0);
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

void DirectCommandList::DrawSinglePrimitiveToWindow(std::shared_ptr<class Window> window, Primitive* primitive)
{
	if (window->_numOfBackBuffers != _numOfAllocators)
	{
		char buffer[200];
		sprintf_s(buffer, 200, "Error: Window's back buffer num %d is not equal to CommandList's allocator num %d !\n", window->_numOfBackBuffers, _numOfAllocators);
		OutputDebugStringA(buffer);
		return;
	}

	CD3DX12_VIEWPORT& viewport = window->_viewport;
	D3D12_RECT& d3d12Rect = window->_d3d12Rect;
	std::vector<ComPtr<ID3D12Resource>>& backBuffers = window->_backBuffers;
	UINT& currentBackBuffer = window->_currentBackBuffer;
	ComPtr<ID3D12DescriptorHeap> descriptorHeap = window->rtv_heap;
	ComPtr<IDXGISwapChain3> swapChain = window->_swapChain;
	auto dsv = window->dsv_heap->GetCPUDescriptorHandleForHeapStart();

	SIZE_T heapSize = Application::GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	ComPtr<ID3D12Resource> backBuffer = backBuffers[currentBackBuffer];
	ComPtr<ID3D12CommandAllocator> allocator = _commandAllocators[currentBackBuffer];
	auto rtv = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	rtv.ptr += currentBackBuffer * heapSize;

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

	primitive->PrepareForDrawing(_commandList);
	primitive->SetRootParams(_commandList);

	_commandList->RSSetViewports(1, &viewport);
	_commandList->RSSetScissorRects(1, &d3d12Rect);
	_commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
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
