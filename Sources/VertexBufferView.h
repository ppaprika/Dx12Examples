#pragma once
#include "UploadBuffer.h"


class VertexBufferView
{
public:
	void Init(size_t size, size_t alignment, size_t stride, const void* resource, std::shared_ptr<UploadBuffer> buffer);

	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const { return vertex_buffer_view_; }
private:
	std::shared_ptr<UploadBuffer::Memory> memory_;
	D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view_ = {};
};
