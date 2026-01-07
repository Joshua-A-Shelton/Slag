#ifndef SLAG_VULKANENVOIRNMENT_H
#define SLAG_VULKANENVOIRNMENT_H
#include <gtest/gtest.h>
#include "GraphicsAPIEnvironment.h"

namespace slag
{
    class VulkanEnvironment: public GraphicsAPIEnvironment
    {
    public:
        VulkanEnvironment();
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
} // slag

#endif //SLAG_VULKANENVOIRNMENT_H
