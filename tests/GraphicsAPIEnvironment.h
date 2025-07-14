#ifndef SLAG_GRAPHICSAPIENVOIRNMENT_H
#define SLAG_GRAPHICSAPIENVOIRNMENT_H

#include <gtest/gtest.h>
#include <slag/Slag.h>

namespace slag
{
    struct ShaderFile
    {
        std::string pathIndicator;
        ShaderStageFlags stage;
    };
    class GraphicsAPIEnvironment: public ::testing::Environment
    {
    public:
        virtual std::unique_ptr<slag::ShaderPipeline> loadPipelineFromFiles(ShaderFile* stages, size_t stageCount,ShaderProperties& properties, VertexDescription& vertexDescription, FrameBufferDescription& framebufferDescription)=0;
        virtual std::unique_ptr<slag::ShaderPipeline> newShaderPipeline(ShaderCode* computeCode)=0;
    };
} // slag

#endif //SLAG_GRAPHICSAPIENVOIRNMENT_H