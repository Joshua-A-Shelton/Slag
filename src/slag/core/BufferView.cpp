#include "BufferView.h"

#include "slag/backends/Backend.h"
namespace slag
{
    BufferView* BufferView::newBufferView(Buffer* buffer, Pixels::Format format, uint64_t offset, uint64_t size)
    {
        return Backend::current()->newBufferView(buffer,format,offset,size);
    }
} // slag