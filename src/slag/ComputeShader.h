#ifndef SLAG_COMPUTESHADER_H
#define SLAG_COMPUTESHADER_H
#include "UniformSet.h"
#include "PushConstantRange.h"
namespace slag
{

    class ComputeShader
    {
    public:
        virtual ~ComputeShader()=default;
        virtual UniformSet* getUniformSet(size_t index)=0;
        virtual size_t uniformSetCount()=0;
        virtual PushConstantRange* getPushConstantRange(size_t index)=0;
        virtual size_t pushConstantRangeCount()=0;
        static ComputeShader* create(const char* shaderFile);
    };

} // slag

#endif //SLAG_COMPUTESHADER_H
