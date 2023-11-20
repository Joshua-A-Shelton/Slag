#ifndef SLAG_VERTEXBUFFER_H
#define SLAG_VERTEXBUFFER_H
#include "Buffer.h"

namespace slag
{
    class VertexBuffer: virtual public Buffer
    {
    public:
        virtual ~VertexBuffer()=default;
        static VertexBuffer* create(void* data, size_t dataLength, Buffer::Usage usage);
        static VertexBuffer* create(size_t size, Buffer::Usage usage);
    };
} // slag
#endif //SLAG_VERTEXBUFFER_H