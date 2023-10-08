#ifndef SLAG_UNIFORMDESCRIPTORSET_H
#define SLAG_UNIFORMDESCRIPTORSET_H
#include <string>
#include "UniformDescriptor.h"
namespace slag
{
    class UniformDescriptorSet
    {
    public:
        virtual ~UniformDescriptorSet()=default;

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
#endif //SLAG_UNIFORMDESCRIPTORSET_H
