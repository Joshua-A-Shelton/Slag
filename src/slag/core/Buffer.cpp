#include "Buffer.h"
#include <slag/backends/Backend.h>
namespace slag
{
    Buffer* Buffer::create(void* data, size_t dataSize, Accessibility accessibility, UsageFlags usage)
    {
        return Backend::current()->newBuffer(data,dataSize,accessibility,usage);
    }

    Buffer* Buffer::create(size_t size, Accessibility accessibility, UsageFlags usage)
    {
        return Backend::current()->newBuffer(size,accessibility,usage);
    }
} // slag
