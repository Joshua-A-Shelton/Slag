#include "Buffer.h"
#include "BackEnd/BackEndLib.h"

namespace slag
{
    Buffer* Buffer::newBuffer(void* data, size_t dataSize, Buffer::Accessibility accessibility, Buffer::Usage usage)
    {
        return lib::BackEndLib::get()->newBuffer(data,dataSize,accessibility,usage);
    }

    Buffer* Buffer::newBuffer(size_t bufferSize, Buffer::Accessibility accessibility, Buffer::Usage usage)
    {
        return lib::BackEndLib::get()->newBuffer(bufferSize,accessibility,usage);
    }
} // slag