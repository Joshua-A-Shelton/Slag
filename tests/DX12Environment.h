#ifndef SLAG_DX12ENVIRONMENT_H
#define SLAG_DX12ENVIRONMENT_H
#include "GraphicsAPIEnvironment.h"

namespace slag
{
    class DX12Environment: public GraphicsAPIEnvironment
    {
    public:
        DX12Environment();
        void SetUp() override;
        void TearDown() override;
        virtual std::unique_ptr<slag::ShaderPipeline> loadPipelineFromFiles(ShaderFile* stages, size_t stageCount,ShaderProperties& properties, VertexDescription& vertexDescription, FrameBufferDescription& framebufferDescription)override;
        virtual std::unique_ptr<slag::ShaderPipeline> loadPipelineFromFiles(ShaderFile* stages, size_t stageCount,ShaderProperties& properties, VertexDescription& vertexDescription, FrameBufferDescription& framebufferDescription,DescriptorIdentity(*identify)(const DescriptorIdentityParameters&, void*), void* identifyData)override;
        virtual std::unique_ptr<slag::ShaderPipeline> loadPipelineFromFiles(ShaderFile& computeCode)override;
        virtual utilities::DescriptorDictionary* getShaderDictionary(const std::string& shaderName)override;
        virtual SDL_WindowFlags windowFlags()override;
    private:
        std::unordered_map<std::string,utilities::DescriptorDictionary> _shaderDictionaries;
    };
}

#endif //SLAG_DX12ENVIRONMENT_H