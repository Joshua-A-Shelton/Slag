#ifndef SLAG_SHADER_H
#define SLAG_SHADER_H

#include "Resource.h"
#include "UniformSet.h"

namespace slag
{
    class Shader: public Resource
    {
    public:
        virtual ~Shader()=default;
        virtual UniformSet* getUniformSet(size_t index)=0;
    };
}
#endif //SLAG_SHADER_H
