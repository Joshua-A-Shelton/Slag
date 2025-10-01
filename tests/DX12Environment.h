#ifndef SLAG_DX12ENVIRONMENT_H
#define SLAG_DX12ENVIRONMENT_H
#include "GraphicsAPIEnvironment.h"

namespace slag
{
    class DX12Environment: public GraphicsAPIEnvironment
    {
        void SetUp() override;
        void TearDown() override;
        virtual std::unique_ptr<slag::ShaderPipeline> loadPipelineFromFiles(ShaderFile* stages, size_t stageCount,ShaderProperties& properties, VertexDescription& vertexDescription, FrameBufferDescription& framebufferDescription)override;
        virtual std::unique_ptr<slag::ShaderPipeline> loadPipelineFromFiles(ShaderFile& computeCode)override;
        virtual SDL_WindowFlags windowFlags()override;
    };
}

#endif //SLAG_DX12ENVIRONMENT_H