#include "CommandList.h"

#include <intsafe.h>

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

void CommandList::SingleAndWait(Microsoft::WRL::ComPtr<ID3D12Fence> fence, UINT fenceValue)
{
	_commandQueue->Signal(fence.Get(), fenceValue);
	if(fence->GetCompletedValue() < fenceValue)
	{
		HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		fence->SetEventOnCompletion(fenceValue, event);
		WaitForSingleObject(event, DWORD_MAX);
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
