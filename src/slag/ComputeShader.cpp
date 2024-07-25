#include <filesystem>
#include <fstream>
#include "ComputeShader.h"
#include "SlagLib.h"
#include "BackEnd/Vulkan/VulkanComputeShader.h"
namespace slag
{
    ComputeShader* ComputeShader::create(const char* shaderFile)
    {
        auto shaderPath = std::filesystem::absolute(shaderFile);


        std::ifstream computeFile(shaderPath, std::ios::ate | std::ios::binary);
        if(!computeFile.is_open())
        {
            throw std::runtime_error("Unable to open vertex file");
        }

        size_t computeSize = (size_t)computeFile.tellg();

        std::vector<char> computebuffer(computeSize);

        computeFile.seekg(0);

        computeFile.read(computebuffer.data(), computeSize);

        computeFile.close();
        switch (SlagLib::usingBackEnd())
        {
            case VULKAN:
#ifdef SLAG_VULKAN_BACKEND
                return new vulkan::VulkanComputeShader(computebuffer);
#endif
                break;
            case DX12:
                break;
        }
        return nullptr;
    }
} // slag