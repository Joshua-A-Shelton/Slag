#include "VulkanEnvironment.h"
#include <slag/Slag.h>
namespace slag
{
    void vulkanDebugHandler(const std::string& message, SlagDebugLevel debugLevel, int32_t messageID)
    {
        std::cout << message << std::endl;
        if (debugLevel != SlagDebugLevel::SLAG_INFO)
        {
            GTEST_FAIL();
        }
    }

    void VulkanEnvironment::SetUp()
    {
#ifndef SLAG_VULKAN_BACKEND
        GTEST_SKIP();
#endif
        Environment::SetUp();
        //clean up previous environment if it exists
        if (slag::slagGraphicsCard()!=nullptr)
        {
            slag::cleanup();
        }
        slag::initialize(SlagInitInfo{.graphicsBackend = GraphicsBackend::VULKAN_GRAPHICS_BACKEND, .slagDebugHandler=vulkanDebugHandler});
        SetAsCurrentEnv();
    }

    void VulkanEnvironment::TearDown()
    {
        if (slag::slagGraphicsCard()!=nullptr)
        {
            slag::cleanup();
        }
        Environment::TearDown();
    }

    std::unique_ptr<slag::ShaderPipeline> VulkanEnvironment::loadPipelineFromFiles(ShaderFile* stages, size_t stageCount,ShaderProperties& properties, VertexDescription& vertexDescription, FrameBufferDescription& framebufferDescription)
    {
        std::vector<ShaderCode> shaderCode;
        std::vector<ShaderCode*> compiledStages(stageCount);
        for (size_t i = 0; i < stageCount; ++i)
        {
            std::vector<unsigned char> compiledStagesCode(stageCount);
            auto path = stages[i].pathIndicator+".spv";
            shaderCode.push_back(ShaderCode(stages[i].stage,ShaderCode::CodeLanguage::SPIRV,path));

        }
        for (size_t i = 0; i < stageCount; ++i)
        {
            compiledStages[i] = &shaderCode[i];
        }
        return std::unique_ptr<slag::ShaderPipeline>(slag::ShaderPipeline::newShaderPipeline(compiledStages.data(),compiledStages.size(),properties,vertexDescription,framebufferDescription));

    }

    std::unique_ptr<slag::ShaderPipeline> VulkanEnvironment::newShaderPipeline(ShaderCode* computeCode)
    {
        return std::unique_ptr<slag::ShaderPipeline>(slag::ShaderPipeline::newShaderPipeline(*computeCode));
    }

    SDL_WindowFlags VulkanEnvironment::windowFlags()
    {
        return static_cast<SDL_WindowFlags>(SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);
    }

} // slag
