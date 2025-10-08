#include "DX12Environment.h"
#include <regex>
namespace slag
{
    bool IN_DX12_ENV_CONSTRUCTOR = false;
    void dx12DebugHandler(const std::string& message, SlagDebugLevel debugLevel, int32_t messageID)
    {
        if (!IN_DX12_ENV_CONSTRUCTOR)
        {

            if (debugLevel != SlagDebugLevel::SLAG_INFO)
            {
                std::cout << message << std::endl;
                GTEST_FAIL();
            }
        }
    }

    void DX12Environment::SetUp()
    {
#ifndef SLAG_DX12_BACKEND
        GTEST_SKIP();
#endif
        IN_DX12_ENV_CONSTRUCTOR = true;
        Environment::SetUp();

        //clean up previous environment if it exists
        if (slag::slagGraphicsCard()!=nullptr)
        {
            slag::cleanup();
        }
        auto result = slag::initialize(SlagInitInfo{.graphicsBackend = GraphicsBackend::DX12_GRAPHICS_BACKEND, .slagDebugHandler=dx12DebugHandler});
        if (result != SlagInitializationResult::SLAG_INITIALIZATION_SUCCESS)
        {
            GTEST_FAIL();
        }
        SetAsCurrentEnv();
        IN_DX12_ENV_CONSTRUCTOR = false;

    }

    std::string dx12ShaderReflectionRename(const DescriptorRenameParameters& renameParams,void* userData)
    {
        std::string returnString = renameParams.originalName.substr(0, renameParams.originalName.find_last_of('_'));
        returnString = std::regex_replace(returnString, std::regex("\\_"), ".");
        return returnString;
    }

    void DX12Environment::TearDown()
    {
        if (slag::slagGraphicsCard()!=nullptr)
        {
            slag::cleanup();
        }
        Environment::TearDown();
    }

    std::unique_ptr<slag::ShaderPipeline> DX12Environment::loadPipelineFromFiles(ShaderFile* stages, size_t stageCount,
        ShaderProperties& properties, VertexDescription& vertexDescription,
        FrameBufferDescription& framebufferDescription)
    {
        std::vector<ShaderCode> shaderCode;
        std::vector<ShaderCode*> compiledStages(stageCount);
        for (size_t i = 0; i < stageCount; ++i)
        {
            std::vector<unsigned char> compiledStagesCode(stageCount);
            auto path = stages[i].pathIndicator+".dxil";
            shaderCode.push_back(ShaderCode(stages[i].stage,ShaderCode::CodeLanguage::DXIL,path));

        }
        for (size_t i = 0; i < stageCount; ++i)
        {
            compiledStages[i] = &shaderCode[i];
        }
        return std::unique_ptr<slag::ShaderPipeline>(slag::ShaderPipeline::newShaderPipeline(compiledStages.data(),compiledStages.size(),properties,vertexDescription,framebufferDescription,dx12ShaderReflectionRename));
    }

    std::unique_ptr<slag::ShaderPipeline> DX12Environment::loadPipelineFromFiles(ShaderFile& computeCode)
    {
        ShaderCode shaderCode(computeCode.stage,ShaderCode::CodeLanguage::SPIRV,computeCode.pathIndicator+".dxil");
        return std::unique_ptr<slag::ShaderPipeline>(slag::ShaderPipeline::newShaderPipeline(shaderCode));
    }

    SDL_WindowFlags DX12Environment::windowFlags()
    {
        return static_cast<SDL_WindowFlags>(SDL_WINDOW_RESIZABLE);
    }
}
