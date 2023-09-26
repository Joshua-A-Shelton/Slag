#ifndef SLAG_UNIFORMBUFFER_H
#define SLAG_UNIFORMBUFFER_H
#include <string>
#include "Uniform.h"
namespace slag
{
    class UniformBuffer
    {
    public:
        virtual ~UniformBuffer()=default;

        enum BufferType
        {
            TEXTURE,
            UNIFORM,
            STORAGE
        };

        virtual BufferType bufferType()=0;
        virtual size_t uniformCount()=0;
        virtual std::string name()=0;

    };
}
#endif //SLAG_UNIFORMBUFFER_H
