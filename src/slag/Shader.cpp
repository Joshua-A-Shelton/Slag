#include <filesystem>
#include <fstream>
#include "Shader.h"
#include "SlagLib.h"
#ifdef SLAG_VULKAN_BACKEND
#include "BackEnd/Vulkan/VulkanShader.h"
#endif

namespace slag
{

    Shader *Shader::create(const char *vertexFile, const char *fragmentFile, FramebufferDescription& framebufferDescription)
    {
        auto vertexPath = std::filesystem::absolute(vertexFile);
        auto fragPath = std::filesystem::absolute(fragmentFile);

        std::ifstream vertFile(vertexPath, std::ios::ate | std::ios::binary);
        if(!vertFile.is_open())
        {
            throw std::runtime_error("Unable to open vertex file");
        }

        size_t vertSize = (size_t)vertFile.tellg();

        std::vector<char> vertbuffer(vertSize);

        vertFile.seekg(0);

        vertFile.read(vertbuffer.data(), vertSize);

        vertFile.close();


        std::ifstream fragFile(fragPath, std::ios::ate | std::ios::binary);
        if(!fragFile.is_open())
        {
            throw std::runtime_error("Unable to open fragment file");
        }

        size_t fragSize = (size_t)fragFile.tellg();

        std::vector<char> fragbuffer(fragSize);

        fragFile.seekg(0);

        fragFile.read(fragbuffer.data(), fragSize);

        fragFile.close();

        switch (SlagLib::usingBackEnd())
        {
            case VULKAN:
#ifdef SLAG_VULKAN_BACKEND
                return new vulkan::VulkanShader(vertbuffer,fragbuffer,framebufferDescription);
#endif
                break;
        }
        return nullptr;
    }
}