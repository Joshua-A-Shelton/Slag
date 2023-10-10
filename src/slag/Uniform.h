#ifndef SLAG_UNIFORM_H
#define SLAG_UNIFORM_H
#include <string>
#include "UniformDescriptor.h"
namespace slag
{
    class Uniform
    {
    public:
        virtual ~Uniform()=default;

        enum UniformType
        {
            TEXTURE,
            UNIFORM,
            STORAGE
        };

        virtual UniformType uniformType()=0;
        virtual size_t descriptorCount()=0;
        const virtual std::string& name()=0;
        virtual uint32_t binding()=0;

    };
}
#endif //SLAG_UNIFORM_H
