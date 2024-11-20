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

	_pageSize = pageSize;
	_device = Device;
}

UploadBuffer::~UploadBuffer()
{
	
}

UploadBuffer::PageTracker::~PageTracker()
{
	if(auto ownedBuffer = _ownedBuffer.lock())
	{
		if(!ownedBuffer->TryReleasePage(_trackingPage))
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


struct UploadBuffer::Memory UploadBuffer::Allocation(size_t size, size_t alignment)
{
	//UploadBuffer::Memory Allocation;

	//Allocation.CPUPtr = static_cast<uint8_t*>(_cpuPtr) + _offSet;
	//Allocation.GPUPtr = _gpuPtr + _offSet;

	//_offSet += size;

	//return Allocation;
}

bool UploadBuffer::TryReleasePage(std::weak_ptr<Page> page)
{
	if(auto lockedPage = page.lock())
	{
		auto it = std::find_if(_usedPages.begin(), _usedPages.end(), 
		[&lockedPage](std::unique_ptr<Page>& usedPage)
			{
				return lockedPage.get() == usedPage.get();
			});

		if(it != _usedPages.end())
		{
			_availablePages.push_front(std::move(*it));

			_usedPages.erase(it);

			return true;
		}
	}

	return false;
}

UploadBuffer::Page::Page(size_t size, ComPtr<ID3D12Device> device)
{
	CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(size);
	ThrowIfFailed(device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&memoryPage)
	));

}
