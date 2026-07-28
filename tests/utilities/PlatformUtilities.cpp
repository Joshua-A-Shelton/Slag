#include "PlatformUtilities.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>
namespace slag::utilities
{
    ShaderModule createShaderModule(slag::GraphicsCard* card, const std::string& shaderIdentifier)
    {
        auto api = slag::Slag::backend()->api();
        std::filesystem::path shaderPath = shaderIdentifier;
        auto language =  slag::ShaderLanguage::CUSTOM;
        switch (api)
        {
        case slag::BackendAPI::VULKAN:
            shaderPath += ".spv";
            language = slag::ShaderLanguage::SPIRV;
            break;
        case slag::BackendAPI::DX12:
            shaderPath += ".dxil";
            language = slag::ShaderLanguage::DXIL;
            break;
        default:
            throw std::runtime_error("Invalid backend API");
        }
        std::ifstream file(shaderPath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file");
        }

        ShaderModule shaderModule{};

        const std::size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        shaderModule.loadedCode.resize(fileSize);
        file.read(reinterpret_cast<char*>(shaderModule.loadedCode.data()), fileSize);
        file.close();

        shaderModule.details.code = shaderModule.loadedCode.data();
        shaderModule.details.codeLength = fileSize;
        shaderModule.details.language = language;
        return shaderModule;
    }

}