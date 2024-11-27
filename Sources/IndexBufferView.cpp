


#include "IndexBufferView.h"

void IndexBufferView::Init(size_t size, size_t alignment, DXGI_FORMAT format, const void* resource,
                                  std::shared_ptr<UploadBuffer> buffer)
{
    memory_ = buffer->Allocation(size, alignment);
    memcpy(memory_->cpu_ptr, resource, size);
    index_buffer_view_.BufferLocation = memory_->gpu_ptr;
    index_buffer_view_.SizeInBytes = size;
    index_buffer_view_.Format = format;
}
