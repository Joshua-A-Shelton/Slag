#include <cassert>
#include "Texture.h"
#include "BackEnd/BackEndLib.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace slag
{

    Texture* Texture::newTexture(void* data, size_t dataSize, Pixels::Format dataFormat, uint32_t width, uint32_t height, uint32_t mipLevels, TextureUsage usage, Texture::Layout initializedLayout,bool generateMipMaps)
    {
        return lib::BackEndLib::get()->newTexture(data,dataSize,dataFormat,dataFormat,width,height,mipLevels,usage,initializedLayout,generateMipMaps);
    }

    Texture* Texture::newTexture(void* data, size_t dataSize, Pixels::Format dataFormat, Pixels::Format textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, TextureUsage usage, Texture::Layout initializedLayout, bool generateMipMaps)
    {
        return lib::BackEndLib::get()->newTexture(data,dataSize,dataFormat,textureFormat,width,height,mipLevels,usage,initializedLayout,generateMipMaps);
    }

    Texture* Texture::newTexture(const std::filesystem::path& imagePath, Pixels::Format textureFormat, uint32_t mipLevels, TextureUsage usage, Texture::Layout initializedLayout, bool generateMipMaps)
    {
        int w, h, channels;
        unsigned char* data = stbi_load(absolute(imagePath).string().c_str(),&w,&h,&channels,4);
        Texture* tex = lib::BackEndLib::get()->newTexture(data,sizeof(unsigned char)*w*h*4,Pixels::Format::R8G8B8A8_UINT,textureFormat,w,h,mipLevels,usage,initializedLayout,generateMipMaps);
        stbi_image_free(data);
        return tex;
    }

    Texture* Texture::newTexture(Color* colorArray, size_t colorCount, Pixels::Format textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, TextureUsage usage, Texture::Layout initializedLayout, bool generateMipMaps)
    {
        return lib::BackEndLib::get()->newTexture(colorArray,sizeof(Color)*colorCount,Pixels::Format::R16G16B16A16_FLOAT,textureFormat,width,height,mipLevels,usage,initializedLayout,generateMipMaps);
    }
} // slag