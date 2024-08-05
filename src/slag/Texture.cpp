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


    Texture *Texture::create(const char *fileLocation, unsigned int mipLevels,Texture::Layout layout, Features features)
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
                    tex= new vulkan::VulkanTexture(width,height,mipLevels,VK_IMAGE_ASPECT_COLOR_BIT,Pixels::PixelFormat::R8G8B8A8_SRGB,data,layout,features, false);
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

    Texture* Texture::create(const char* fileLocation, Pixels::PixelFormat textureFormat, unsigned int mipLevels, Texture::Layout layout, Texture::Features features)
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
                    tex= new vulkan::VulkanTexture(width,height,mipLevels,VK_IMAGE_ASPECT_COLOR_BIT,Pixels::PixelFormat::R8G8B8A8_SRGB,data,textureFormat,layout,features, false);
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

    Texture *Texture::create(uint32_t width, uint32_t height, Pixels::PixelFormat textureFormat, Texture::Layout layout, uint32_t mipLevels, Features features)
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
                tex= new vulkan::VulkanTexture(width, height, mipLevels, VK_IMAGE_ASPECT_COLOR_BIT, textureFormat, layout, features, false);
#endif
                break;
            case BackEnd::DX12:
                break;
        }
        return tex;
    }

    Texture *Texture::create(uint32_t width, uint32_t height, Pixels::PixelFormat pixelFormat, void *pixelData, uint32_t mipLevels, Texture::Layout layout, Features features)
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
                tex= new vulkan::VulkanTexture(width, height, mipLevels, VK_IMAGE_ASPECT_COLOR_BIT, pixelFormat, pixelData, layout, features, false);
#endif
                break;
            case BackEnd::DX12:
                break;
        }

        return tex;
    }

    Texture* Texture::create(uint32_t width, uint32_t height, Pixels::PixelFormat textureFormat, Pixels::PixelFormat pixelFormat, void* pixelData, uint32_t mipLevels, Texture::Layout layout,
                             Texture::Features features)
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
                tex= new vulkan::VulkanTexture(width, height, mipLevels, VK_IMAGE_ASPECT_COLOR_BIT, pixelFormat, pixelData, textureFormat, layout, features, false);
#endif
                break;
            case BackEnd::DX12:
                break;
        }

        return tex;
    }

    Texture* Texture::create(ColorArray& colors, uint32_t mipLevels, Texture::Layout layout, Texture::Features features)
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
            tex= new vulkan::VulkanTexture(colors.width(),colors.height(),mipLevels,VK_IMAGE_ASPECT_COLOR_BIT,Pixels::PixelFormat::R32G32B32A32_SFLOAT,&colors.at(0,0),layout,features, false);
#endif
                break;
            case BackEnd::DX12:
                break;
        }

        return tex;
    }

    Texture* Texture::create(ColorArray& colors, Pixels::PixelFormat textureFormat, uint32_t mipLevels, Texture::Layout layout, Texture::Features features)
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
                tex= new vulkan::VulkanTexture(colors.width(),colors.height(),mipLevels,VK_IMAGE_ASPECT_COLOR_BIT,Pixels::PixelFormat::R32G32B32A32_SFLOAT,&colors.at(0,0),textureFormat,layout,features, false);
#endif
                break;
            case BackEnd::DX12:
                break;
        }

        return tex;
    }


}