#include "VertexBufferView.h"

void VertexBufferView::Init(size_t size, size_t alignment, size_t stride, const void* resource, std::shared_ptr<UploadBuffer> buffer)
{
	memory_ = buffer->Allocation(size, alignment);
	memcpy(memory_->cpu_ptr, resource, size);
	vertex_buffer_view_.BufferLocation = memory_->gpu_ptr;
	vertex_buffer_view_.SizeInBytes = size;
	vertex_buffer_view_.StrideInBytes = stride;
}