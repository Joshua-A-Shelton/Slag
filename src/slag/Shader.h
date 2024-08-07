#ifndef SLAG_SHADER_H
#define SLAG_SHADER_H

#include "UniformSet.h"
#include "FramebufferDescription.h"
#include "PushConstantRange.h"
#include "VertexDescription.h"

namespace slag
{
    class Shader
    {
    public:
        virtual ~Shader()=default;
        virtual UniformSet* getUniformSet(size_t index)=0;
        virtual PushConstantRange* getPushConstantRange(size_t index)=0;
        virtual size_t pushConstantRangeCount()=0;
        static Shader* create(const char* vertexFile, const char* fragmentFile, FramebufferDescription& framebufferDescription);
        static Shader* create(const char* vertexFile, const char* fragmentFile, VertexDescription& vertexDescription, FramebufferDescription& framebufferDescription);
    };
}
#endif //SLAG_SHADER_H
