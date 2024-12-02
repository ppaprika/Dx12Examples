#pragma once
#include <deque>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>
#include <D3DX12/d3d12.h>
#include <wrl/client.h>

#include "Defines.h"

class DirectCommandList;
using Microsoft::WRL::ComPtr;

class UploadBuffer : public std::enable_shared_from_this<UploadBuffer>
{
	/* new version */
public:
	struct Page;

	struct PageTracker
	{
		explicit PageTracker(std::weak_ptr<Page> page, std::weak_ptr<UploadBuffer> buffer)
			: tracking_page_(std::move(page)), owned_buffer_(std::move(buffer)) {}

		PageTracker(const PageTracker&) = delete;
		PageTracker& operator=(const PageTracker&) = delete;
		PageTracker(PageTracker&&) = delete;
		PageTracker& operator=(PageTracker&&) = delete;

		~PageTracker();

	private:
		std::weak_ptr<Page> tracking_page_;
		std::weak_ptr<UploadBuffer> owned_buffer_;
	};
	friend PageTracker;

	struct Memory
	{
		Memory(void* cpuPtr, D3D12_GPU_VIRTUAL_ADDRESS gpuPtr, std::shared_ptr<PageTracker> tracker)
			: cpu_ptr(cpuPtr), gpu_ptr(gpuPtr), tracker_(std::move(tracker)) {}

		void* cpu_ptr;
		D3D12_GPU_VIRTUAL_ADDRESS gpu_ptr;
	private:
		std::shared_ptr<PageTracker> tracker_;
	};

	UploadBuffer(ComPtr<ID3D12Device> device, size_t pageSize = _2MB);

	virtual ~UploadBuffer();

	std::shared_ptr<Memory> Allocation(size_t size, size_t alignment = 0);

private:
	bool TryReleasePage(std::weak_ptr<Page> page);

	struct Page
	{
		Page(size_t size, ComPtr<ID3D12Device> device);

		ComPtr<ID3D12Resource> memory_page;

		void* cpu_ptr;

		D3D12_GPU_VIRTUAL_ADDRESS gpu_ptr;

		size_t off_set = 0;

		std::weak_ptr<PageTracker> track_this_page;

		size_t page_size;

		void Reset();

		bool HasEnoughSpace(size_t size, size_t alignment);
	};

	std::deque<std::shared_ptr<Page>> available_pages_;
	std::vector<std::shared_ptr<Page>> used_pages_;
	std::weak_ptr<Page> current_page_;

	ComPtr<ID3D12Device> device_;
	size_t page_size_ = 0;

	/* new version */
};
