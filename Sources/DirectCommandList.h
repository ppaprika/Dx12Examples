#pragma once
#include <memory>
#include <vector>
#include <D3DX12/d3d12.h>
#include <wrl/client.h>

#include "Window.h"

struct CreateWindowParams;
class Primitive;
using Microsoft::WRL::ComPtr;


class DirectCommandList : public std::enable_shared_from_this<DirectCommandList>
{
public:
	DirectCommandList(ComPtr<ID3D12Device> device, D3D12_COMMAND_LIST_TYPE type, int numOfAllocators);
	void CreateTargetWindow(CreateWindowParams* Params);
	~DirectCommandList();


	void SingleAndWait();

	ComPtr<ID3D12GraphicsCommandList2> GetCommandList() { return _commandList; }

	void DrawSinglePrimitive(Primitive* primitive);

	void Reset();
	void Draw(Primitive* primitive);
	void Present();

	int GetTargetWindowWidth() const;
	int GetTargetWindowHeight() const;
	std::shared_ptr<Window> GetTargetWindow() { return window; }

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

	std::shared_ptr<Window> window;

	ComPtr<ID3D12Fence> _fence;
	UINT _fenceValue = 0;
	std::vector<UINT> _waitingValue;

	// from outside
	ComPtr<ID3D12Device> _device;
	std::shared_ptr<Window> current_window;
};
