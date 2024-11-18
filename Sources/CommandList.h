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

	void Execute();
	void SingleAndWait(ComPtr<ID3D12Fence> fence, UINT fenceValue);

	ComPtr<ID3D12GraphicsCommandList2> GetCommansList() { return _commandList; }

	void DrawToWindow(std::shared_ptr<class Window> Window, DrawWindowParams& Params);

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
