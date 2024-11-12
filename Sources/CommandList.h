#pragma once
#include <vector>
#include <D3DX12/d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class CommandList
{
public:
	CommandList(ComPtr<ID3D12Device> device, D3D12_COMMAND_LIST_TYPE type, int numOfAllocators);
	~CommandList();

	void Execute();
	void SingleAndWait(ComPtr<ID3D12Fence> fence, UINT fenceValue);

	ComPtr<ID3D12GraphicsCommandList2> GetCommansList() { return _commandList; }

public:
	void InitCommandQueue();
	void InitAllocators();
	void InitCommandList();

	D3D12_COMMAND_LIST_TYPE _type;
	int _numOfAllocators;

	// owned by this
	std::vector<ComPtr<ID3D12CommandAllocator>> _commandAllocators;
	ComPtr<ID3D12GraphicsCommandList2> _commandList;
	ComPtr<ID3D12CommandQueue> _commandQueue;

	// from outside
	ComPtr<ID3D12Device> _device;
};
