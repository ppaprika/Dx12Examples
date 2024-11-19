#pragma once
#include <memory>
#include <D3DX12/d3d12.h>
#include <wrl/client.h>

#include "Defines.h"

class CommandList;
using Microsoft::WRL::ComPtr;

class UploadBuffer
{
	/* old version */
//private:
//	ComPtr<ID3D12Device> _device;
//
//	std::shared_ptr<CommandList> _commandList;
//
//public:
//	UploadBuffer(ComPtr<ID3D12Device> Device);
//
//	void DoUpload(ComPtr<ID3D12Resource>& DestinationResource, size_t numElement, size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags);
	/* old version */


	/* new version */
public:
	UploadBuffer(ComPtr<ID3D12Device> Device, size_t pageSize = _2MB);

	struct Memory
	{
		void* CPUPtr;
		D3D12_GPU_VIRTUAL_ADDRESS GPUPtr;
	};

	Memory Allocation(size_t size);

private:
	void* _cpuPtr;

	D3D12_GPU_VIRTUAL_ADDRESS _gpuPtr;

	//std::unique_ptr<CommandList> _commandList;

	ComPtr<ID3D12Resource> _buffer;

	ComPtr<ID3D12Device> _device;

	size_t _offSet = 0;

	/* new version */
};
