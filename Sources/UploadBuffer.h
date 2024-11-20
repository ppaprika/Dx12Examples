#pragma once
#include <deque>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>
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
	struct Page;

	struct PageTracker
	{
		explicit PageTracker(std::weak_ptr<Page> page, std::weak_ptr<UploadBuffer> buffer)
			: _trackingPage(std::move(page)), _ownedBuffer(std::move(buffer)) {}

		~PageTracker();

	private:
		std::weak_ptr<Page> _trackingPage;
		std::weak_ptr<UploadBuffer> _ownedBuffer;
	};
	friend PageTracker;

	struct Memory
	{
		Memory(void* cpuPtr, D3D12_GPU_VIRTUAL_ADDRESS gpuPtr, std::shared_ptr<PageTracker> tracker)
			: CPUPtr(cpuPtr), GPUPtr(gpuPtr), _tracker(std::move(tracker)) {}

		void* CPUPtr;
		D3D12_GPU_VIRTUAL_ADDRESS GPUPtr;
	private:
		std::shared_ptr<PageTracker> _tracker;
	};

	UploadBuffer(ComPtr<ID3D12Device> device, size_t pageSize = _2MB);

	virtual ~UploadBuffer();

	Memory Allocation(size_t size, size_t alignment = 0);

private:
	bool TryReleasePage(std::weak_ptr<Page> page);

	struct Page
	{
		Page(size_t size, ComPtr<ID3D12Device> device);

		ComPtr<ID3D12Resource> memoryPage;

		std::weak_ptr<PageTracker> trackThisPage;
	};

	std::deque<std::unique_ptr<Page>> _availablePages;
	std::vector<std::unique_ptr<Page>> _usedPages;
	std::weak_ptr<Page> _currentPage;

	void* _cpuPtr;

	D3D12_GPU_VIRTUAL_ADDRESS _gpuPtr;

	ComPtr<ID3D12Resource> _buffer;

	ComPtr<ID3D12Device> _device;

	size_t _offSet = 0;

	size_t _pageSize = 0;

	/* new version */
};
