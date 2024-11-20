#pragma once
#include <functional>
#include <memory>
#include <vector>
#include <D3DX12/d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

struct DrawWindowParams
{
	ComPtr<ID3D12PipelineState> PSO = nullptr;
	ComPtr<ID3D12RootSignature> RootSignature = nullptr;

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView = {};
	D3D12_INDEX_BUFFER_VIEW IndexBufferView = {};

	SIZE_T DrawNum = 0;

	std::function<void(ComPtr<ID3D12GraphicsCommandList2>)> SetRootConstant;
};

class CommandList
{
public:
	CommandList(ComPtr<ID3D12Device> device, D3D12_COMMAND_LIST_TYPE type, int numOfAllocators);
	~CommandList();

	//void Execute();
	//void Reset();

	void SingleAndWait();

	ComPtr<ID3D12GraphicsCommandList2> GetCommandList() { return _commandList; }

	void DrawToWindow(std::shared_ptr<class Window> Window, DrawWindowParams& Params);

public:
	void InitCommandQueue();
	void InitAllocators();
	void InitCommandList();
	void InitFence();

	D3D12_COMMAND_LIST_TYPE _type;
	UINT _numOfAllocators;

	// owned by this
	std::vector<ComPtr<ID3D12CommandAllocator>> _commandAllocators;
	ComPtr<ID3D12GraphicsCommandList2> _commandList;
	ComPtr<ID3D12CommandQueue> _commandQueue;

	ComPtr<ID3D12Fence> _fence;
	UINT _fenceValue = 0;
	std::vector<UINT> _waitingValue;

	// from outside
	ComPtr<ID3D12Device> _device;
};
