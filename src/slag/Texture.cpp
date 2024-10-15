#include <cassert>
#include "Texture.h"
#include "BackEnd/BackEndLib.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace slag
{

    Texture* Texture::newTexture(void* data, size_t dataSize, Pixels::Format dataFormat, Texture::Type type, uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t layers, uint8_t sampleCount,
                                 TextureUsage usage, Texture::Layout initializedLayout)
    {
        return lib::BackEndLib::get()->newTexture(data,dataSize,dataFormat,type,width,height,mipLevels,layers,sampleCount,usage,initializedLayout);
    }

    Texture* Texture::newTexture(const std::filesystem::path& imagePath, uint32_t mipLevels, TextureUsage usage, bool sRGB, Layout initializedLayout)
    {
        int w, h, channels;
        unsigned char* data = stbi_load(absolute(imagePath).string().c_str(),&w,&h,&channels,4);
        Texture* tex = lib::BackEndLib::get()->newTexture((void**)(&data),1,w*h*Pixels::pixelBytes(Pixels::Format::R8G8B8A8_UINT), sRGB? Pixels::Format::R8G8B8A8_UNORM_SRGB : Pixels::Format::R8G8B8A8_UINT,Texture::Type::TEXTURE_2D,w,h,mipLevels,usage,initializedLayout);
        stbi_image_free(data);
        return tex;
    }

    Texture* Texture::newTexture(Pixels::Format format, size_t width, size_t height, size_t sampleCount, TextureUsage usage, Texture::Layout initializedLayout)
    {
        return lib::BackEndLib::get()->newTexture(nullptr,0,format,TEXTURE_2D,width,height,1,1,sampleCount,usage,initializedLayout);
    }

} // slag