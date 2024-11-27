#include "UploadBuffer.h"

#include <D3DX12/d3dx12_core.h>
#include <D3DX12/d3dx12_resource_helpers.h>

#include "CommandList.h"
#include "Helpers.h"



UploadBuffer::UploadBuffer(ComPtr<ID3D12Device> device, size_t pageSize)
{
	page_size_ = pageSize;
	device_ = device;
}

UploadBuffer::~UploadBuffer()
{
	
}

UploadBuffer::PageTracker::~PageTracker()
{
	if(auto ownedBuffer = owned_buffer_.lock())
	{
		if(!ownedBuffer->TryReleasePage(tracking_page_))
		{
			MessageBox(nullptr, L"Try free GPU memory repeatedly!", L"GPU Memory Error", MB_ICONERROR);
			ExitProcess(1);
		}
	}
	else
	{
		MessageBox(nullptr, L"GPU UploadBuffer not exist!", L"GPU Memory Error", MB_ICONERROR);
		ExitProcess(1);
	}
}


std::shared_ptr<UploadBuffer::Memory> UploadBuffer::Allocation(size_t size, size_t alignment)
{
	if(size > page_size_)
	{
		MessageBox(nullptr, L"Memory required is bigger than page size!", L"Bad Allocate!", MB_ICONERROR);
		ExitProcess(1);
	}

	// current page have enough space
	if(auto page = current_page_.lock())
	{
		if(page->HasEnoughSpace(size, alignment))
		{
			size_t newOffset = AlignUp(page->off_set, alignment);
			std::shared_ptr<Memory> memory = std::make_shared<Memory>(static_cast<uint8_t*>(page->cpu_ptr) + newOffset, page->gpu_ptr + newOffset, page->track_this_page.lock());
			page->off_set = page->off_set + size;
			return memory;
		}
	}

	// current page do not have enough space
	// and we have available page
	if(!available_pages_.empty())
	{
		std::shared_ptr<Page> page = available_pages_.front();
		current_page_ = page;
		available_pages_.pop_front();
		// new tracker to track this page
		std::shared_ptr<PageTracker> tracker = std::make_shared<PageTracker>(page, shared_from_this());
		page->track_this_page = tracker;
		size_t newOffset = AlignUp(page->off_set, alignment);
		std::shared_ptr<Memory> memory = std::make_shared<Memory>(static_cast<uint8_t*>(page->cpu_ptr) + newOffset, page->gpu_ptr + newOffset, tracker);
		page->off_set = page->off_set + size;
		return memory;
	}


	// we do not have available page, new one
	std::shared_ptr<Page> page = std::make_shared<Page>(page_size_, device_);
	current_page_ = page;
	used_pages_.push_back(page);
	std::shared_ptr<PageTracker> tracker = std::make_shared<PageTracker>(page, shared_from_this());
	page->track_this_page = tracker;
	size_t newOffset = AlignUp(page->off_set, alignment);
	std::shared_ptr<Memory> memory = std::make_shared<Memory>(static_cast<uint8_t*>(page->cpu_ptr) + newOffset, page->gpu_ptr + newOffset, tracker);
	page->off_set = page->off_set + size;
	return memory;
}

bool UploadBuffer::TryReleasePage(std::weak_ptr<Page> page)
{
	if(auto lockedPage = page.lock())
	{
		lockedPage->Reset();

		auto it = std::find_if(used_pages_.begin(), used_pages_.end(), 
		[&lockedPage](std::shared_ptr<Page>& usedPage)
			{
				return lockedPage.get() == usedPage.get();
			});

		if(it != used_pages_.end())
		{
			available_pages_.push_back(std::move(*it));

			used_pages_.erase(it);

			return true;
		}
	}

	return false;
}

UploadBuffer::Page::Page(size_t size, ComPtr<ID3D12Device> device)
{
	page_size = size;
	CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(size);
	ThrowIfFailed(device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&memory_page)
	));

	gpu_ptr = memory_page->GetGPUVirtualAddress();
	memory_page->Map(0, nullptr, &cpu_ptr);
}

void UploadBuffer::Page::Reset()
{
	// todo: maybe we don't have to unmap and then remap;
	memory_page->Unmap(0, nullptr);
	gpu_ptr = memory_page->GetGPUVirtualAddress();
	memory_page->Map(0, nullptr, &cpu_ptr);
	off_set = 0;
}

bool UploadBuffer::Page::HasEnoughSpace(size_t size, size_t alignment)
{
	size_t newOffset = AlignUp(off_set, alignment);
	return size + newOffset < page_size;
}
