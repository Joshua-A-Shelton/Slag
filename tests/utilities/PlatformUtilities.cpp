#include "PlatformUtilities.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>

std::unique_ptr<slag::ShaderModule> slagTestsCreateShaderModule(slag::GraphicsCard* card, const std::string& shaderIdentifier)
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

    const std::size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<std::byte> buffer(fileSize);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    file.close();
    return std::unique_ptr<slag::ShaderModule>(card->newShaderModule(language, buffer.data(), fileSize));
}
