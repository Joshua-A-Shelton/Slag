#ifndef SLAGNEXT_UNIFORMSET_H
#define SLAGNEXT_UNIFORMSET_H
#include "UniformBuffer.h"
namespace slag
{
    class UniformSet
    {
    public:
        virtual ~UniformSet()=default;
        virtual size_t index()=0;
        virtual size_t bufferCount()=0;
        virtual UniformBuffer* operator[](size_t index)=0;
    };
}
#endif //SLAGNEXT_UNIFORMSET_H
