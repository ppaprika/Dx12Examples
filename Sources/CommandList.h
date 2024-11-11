#pragma once
#include <vector>
#include <D3DX12/d3d12.h>
#include <wrl/client.h>

class CommandList
{
public:
	CommandList(Microsoft::WRL::ComPtr<ID3D12Device> device, D3D12_COMMAND_LIST_TYPE type, int numOfAllocators);
	~CommandList();

	void Execute();
	void SingleAndWait(Microsoft::WRL::ComPtr<ID3D12Fence> fence, UINT fenceValue);

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> GetCommansList() { return _commandList; }

private:
	void InitCommandQueue();
	void InitAllocators();
	void InitCommandList();

	D3D12_COMMAND_LIST_TYPE _type;
	int _numOfAllocators;

	// owned by this
	std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> _commandAllocators;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> _commandList;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> _commandQueue;
	Microsoft::WRL::ComPtr<ID3D12Fence> _fence;

	// from outside
	Microsoft::WRL::ComPtr<ID3D12Device> _device;
};
