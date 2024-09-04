#include "CommandQueue.h"
#include <cassert>
#include <chrono>
#include "Helpers.h"


CommandQueue::CommandQueue(Microsoft::WRL::ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
{
	m_CommandListType = type;
	m_d3d12Device = device;

	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = m_CommandListType;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	ThrowIfFailed(m_d3d12Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_d3d12CommandQueue)));

	m_FenceValue = 0;
	ThrowIfFailed(m_d3d12Device->CreateFence(m_FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_d3d12Fence)));
	m_FenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(m_FenceEvent && "Failed to create fence event.");
}

CommandQueue::~CommandQueue()
{
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> CommandQueue::GetCommandList()
{
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> value;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;

	if(m_CommandAllocatorQueue.empty())
	{
		allocator = CreateCommandAllocator();
	}
	else
	{
		CommandAllocatorEntry entry = m_CommandAllocatorQueue.front();
		if(IsFenceComplete(entry.fenceValue))
		{
			allocator = entry.commandAllocator;
			allocator->Reset();
			m_CommandAllocatorQueue.pop();
		}
		else
		{
			allocator = CreateCommandAllocator();
		}
	}


	if(!m_CommandListQueue.empty())
	{
		value = m_CommandListQueue.front();
		m_CommandListQueue.pop();

		value->Reset(allocator.Get(), nullptr);
	}
	else
	{
		value = CreateCommandList(allocator);
	}

	ThrowIfFailed(value->SetPrivateDataInterface(__uuidof(allocator), allocator.Get()));
	return value;
}

uint64_t CommandQueue::ExecuteCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList)
{
	commandList->Close();

	ID3D12CommandList* const commandLists[] = { commandList.Get() };
	ID3D12CommandAllocator* allocator;
	UINT dataSize = sizeof(allocator);

	ThrowIfFailed(commandList->GetPrivateData(__uuidof(allocator), &dataSize, &allocator));

	m_d3d12CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
	Signal();

	m_CommandAllocatorQueue.push({m_FenceValue, allocator});
	m_CommandListQueue.push(commandList);

	// ? must be released here?
	allocator->Release();

	return m_FenceValue;
}

uint64_t CommandQueue::Signal()
{
	m_FenceValue++;
	ThrowIfFailed(m_d3d12CommandQueue->Signal(m_d3d12Fence.Get(), m_FenceValue));
	return m_FenceValue;
}

bool CommandQueue::IsFenceComplete(uint64_t fenceValue)
{
	if(m_CommandAllocatorQueue.empty())
	{
		return false;
	}
	else
	{
		//return m_CommandAllocatorQueue.front().fenceValue > fenceValue;
		return m_d3d12Fence->GetCompletedValue() > fenceValue;
	}
}

#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

void CommandQueue::WaitForFenceValue(uint64_t fenceValue)
{
	if(!IsFenceComplete(fenceValue))
	{
		ThrowIfFailed(m_d3d12Fence->SetEventOnCompletion(fenceValue, m_FenceEvent));
		::WaitForSingleObject(m_FenceEvent, DWORD_MAX);
	}
}

void CommandQueue::Flush()
{
	WaitForFenceValue(Signal());
}

Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandQueue::CreateCommandAllocator()
{
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
	ThrowIfFailed(m_d3d12Device->CreateCommandAllocator(m_CommandListType, IID_PPV_ARGS(&allocator)));

	return allocator;
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> CommandQueue::CreateCommandList(
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator)
{
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList;
	ThrowIfFailed(m_d3d12Device->CreateCommandList(0, m_CommandListType, allocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

	return commandList;
}


