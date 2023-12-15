#include <filesystem>
#include "Texture.h"
#include "SlagLib.h"
#ifdef SLAG_VULKAN_BACKEND
#include "BackEnd/Vulkan/VulkanTexture.h"
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace slag
{

    Texture *Texture::create(const char *fileLocation, unsigned int mipLevels, bool renderTargetCapable)
    {
        if(mipLevels == 0)
        {
            mipLevels = 1;
        }
        auto imagePath = std::filesystem::absolute(fileLocation).string();
        int width, height, channels;
        unsigned char* data = stbi_load(imagePath.c_str(),&width,&height,&channels,4);
        Texture* tex = nullptr;
        if(data)
        {
            switch (SlagLib::usingBackEnd())
            {
                case BackEnd::VULKAN:
#ifdef SLAG_VULKAN_BACKEND
                    tex= new vulkan::VulkanTexture(width,height,mipLevels,VK_IMAGE_ASPECT_COLOR_BIT,Pixels::PixelFormat::R8G8B8A8_SRGB,data, false);
#endif
                    break;
                case BackEnd::DX12:
                    break;
            }
            stbi_image_free(data);
        }
        assert(data && "image could not be loaded");
        return tex;
    }

    Texture *Texture::create(uint32_t width, uint32_t height, Pixels::PixelFormat format, void *pixelData, uint32_t mipLevels, bool renderTargetCapable)
    {
        Texture* tex = nullptr;
        if(mipLevels == 0)
        {
            mipLevels = 1;
        }
        switch (SlagLib::usingBackEnd())
        {
            case BackEnd::VULKAN:
#ifdef SLAG_VULKAN_BACKEND
                tex= new vulkan::VulkanTexture(width,height,mipLevels,VK_IMAGE_ASPECT_COLOR_BIT,format,pixelData, renderTargetCapable, false);
#endif
                break;
            case BackEnd::DX12:
                break;
        }

        return tex;
    }
}