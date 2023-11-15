#ifndef SLAG_INDEXBUFFER_H
#define SLAG_INDEXBUFFER_H

#include "Buffer.h"

namespace slag
{
    class IndexBuffer: virtual public Buffer
    {
    public:
        virtual ~IndexBuffer()=default;
        static IndexBuffer* create(void* data, size_t dataLength, Buffer::Usage usage);
    };
} // slag
#endif //SLAG_INDEXBUFFER_H