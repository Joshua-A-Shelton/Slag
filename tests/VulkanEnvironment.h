#ifndef SLAG_VULKANENVOIRNMENT_H
#define SLAG_VULKANENVOIRNMENT_H
#include <gtest/gtest.h>
#include "GraphicsAPIEnvironment.h"

namespace slag
{
    class VulkanEnvironment: public GraphicsAPIEnvironment
    {
        void SetUp() override;
        void TearDown() override;
        virtual std::unique_ptr<slag::ShaderPipeline> loadPipelineFromFiles(ShaderFile* stages, size_t stageCount,ShaderProperties& properties, VertexDescription& vertexDescription, FrameBufferDescription& framebufferDescription)override;
        virtual std::unique_ptr<slag::ShaderPipeline> newShaderPipeline(ShaderCode* computeCode)override;
        virtual SDL_WindowFlags windowFlags()override;
    };
} // slag

#endif //SLAG_VULKANENVOIRNMENT_H
