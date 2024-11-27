#pragma once

#include "UploadBuffer.h"


class IndexBufferView
{
public:
	void Init(size_t size, size_t alignment, DXGI_FORMAT format, const void* resource, std::shared_ptr<UploadBuffer> buffer);

	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const { return index_buffer_view_; }
private:
	std::shared_ptr<UploadBuffer::Memory> memory_;
	D3D12_INDEX_BUFFER_VIEW index_buffer_view_ = {};
};

