#pragma once
#include <vector>
#include <D3DX12/d3d12.h>
#include <wrl/client.h>

class CommandList
{
public:



private:
	// owned by this
	std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> _commandAllocator;
	Microsoft::WRL::ComPtr<ID3D12CommandList> _commandList;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> _commandQueue;

	// from outside
	Microsoft::WRL::ComPtr<ID3D12Device> _device;
};
