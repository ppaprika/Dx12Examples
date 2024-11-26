#include "UploadBuffer.h"

#include <D3DX12/d3dx12_core.h>
#include <D3DX12/d3dx12_resource_helpers.h>

#include "CommandList.h"
#include "Helpers.h"


//UploadBuffer::UploadBuffer(ComPtr<ID3D12Device> Device)
//{
//	_device = Device;
//	_commandList = std::make_shared<CommandList>(_device, D3D12_COMMAND_LIST_TYPE_COPY, 1);
//}
//
//void UploadBuffer::DoUpload(ComPtr<ID3D12Resource>& destinationResource, size_t numElement, size_t elementSize,
//	const void* bufferData, D3D12_RESOURCE_FLAGS flags)
//{
//	size_t bufferSize = numElement * elementSize;
//
//	CD3DX12_HEAP_PROPERTIES destHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
//	CD3DX12_RESOURCE_DESC destResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags);
//	ThrowIfFailed(_device->CreateCommittedResource(
//		&destHeapProperties,
//		D3D12_HEAP_FLAG_NONE,
//		&destResourceDesc,
//		D3D12_RESOURCE_STATE_COMMON,
//		nullptr,
//		IID_PPV_ARGS(&destinationResource)
//	));
//
//
//	if(bufferData)
//	{
//		ComPtr<ID3D12Resource> intermediateResource;
//		CD3DX12_HEAP_PROPERTIES intermediateHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
//		CD3DX12_RESOURCE_DESC intermediateResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
//		ThrowIfFailed(_device->CreateCommittedResource(
//			&intermediateHeapProperties,
//			D3D12_HEAP_FLAG_NONE,
//			&intermediateResourceDesc,
//			D3D12_RESOURCE_STATE_COMMON,
//			nullptr,
//			IID_PPV_ARGS(&intermediateResource)
//		));
//		D3D12_SUBRESOURCE_DATA data;
//		data.pData = bufferData;
//
//		UpdateSubresources(_commandList->GetCommansList().Get(), destinationResource.Get(), intermediateResource.Get(), 0, 0, 1, &data);
//		_commandList->Execute();
//		_commandList->SingleAndWait();
//		_commandList->Reset();
//	}
//}


UploadBuffer::UploadBuffer(ComPtr<ID3D12Device> device, size_t pageSize)
{
	//_device = Device;

	//CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	//CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(pageSize);
	//ThrowIfFailed(_device->CreateCommittedResource(
	//	&heapProperties,
	//	D3D12_HEAP_FLAG_NONE,
	//	&resDesc,
	//	D3D12_RESOURCE_STATE_COMMON,
	//	nullptr,
	//	IID_PPV_ARGS(&_buffer)
	//));

	//_gpuPtr = _buffer->GetGPUVirtualAddress();
	//_buffer->Map(0, nullptr, &_cpuPtr);

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
	//UploadBuffer::Memory Allocation;

	//Allocation.CPUPtr = static_cast<uint8_t*>(_cpuPtr) + _offSet;
	//Allocation.GPUPtr = _gpuPtr + _offSet;

	//_offSet += size;

	//return Allocation;

	// current page have enough space
	if(auto page = current_page_.lock())
	{
		if(page->HasEnoughSpace(size, alignment))
		{
			std::shared_ptr<Memory> memory = std::make_shared<Memory>(static_cast<uint8_t*>(page->cpu_ptr) + page->off_set, page->gpu_ptr + page->off_set, page->track_this_page.lock());
			page->off_set = page->off_set + AlignUp(size, alignment);
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
		std::shared_ptr<Memory> memory = std::make_shared<Memory>(static_cast<uint8_t*>(page->cpu_ptr) + page->off_set, page->gpu_ptr + page->off_set, tracker);
		page->off_set = page->off_set + AlignUp(size, alignment);
		return memory;
	}


	// we do not have available page, new one
	std::shared_ptr<Page> page = std::make_shared<Page>(page_size_, device_);
	current_page_ = page;
	used_pages_.push_back(page);
	std::shared_ptr<PageTracker> tracker = std::make_shared<PageTracker>(page, shared_from_this());
	page->track_this_page = tracker;
	std::shared_ptr<Memory> memory = std::make_shared<Memory>(static_cast<uint8_t*>(page->cpu_ptr) + page->off_set, page->gpu_ptr + page->off_set, tracker);
	page->off_set = page->off_set + AlignUp(size, alignment);
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
	size_t requiredSize = AlignUp(size, alignment);
	return off_set + requiredSize < page_size;
}
