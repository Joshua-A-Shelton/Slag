#include <cassert>
#include "Texture.h"
#include "BackEnd/BackEndLib.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace slag
{

    Texture* Texture::newTexture(const std::filesystem::path& imagePath, Pixels::Format dataFormat, uint32_t mipLevels, TextureUsage usage, Layout initializedLayout)
    {
        bool valid = false;
        switch (dataFormat)
        {
            case Pixels::R8G8B8A8_UINT:
            case Pixels::R8G8B8A8_UNORM:
            case Pixels::R8G8B8A8_UNORM_SRGB:
                valid=true;
                break;
        }
        if(!valid)
        {
            throw std::runtime_error("invalid data format, please pick 4 component, 8 bit, unsigned format");
        }
        int w, h, channels;
        Texture* tex = nullptr;
        unsigned char* data = stbi_load(absolute(imagePath).string().c_str(),&w,&h,&channels,4);
        try
        {
            tex = lib::BackEndLib::get()->newTexture((void**) (&data), 1, w * h * Pixels::pixelBytes(Pixels::Format::R8G8B8A8_UINT), dataFormat, Texture::Type::TEXTURE_2D, w, h, mipLevels, usage,
                                                              initializedLayout);
        }
        catch(std::runtime_error& error)
        {
            stbi_image_free(data);
            throw error;
        }
        stbi_image_free(data);
        return tex;
    }

    Texture* Texture::newTexture(Pixels::Format dataFormat, Texture::Type type, uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t layers, uint8_t sampleCount, TextureUsage usage)
    {
        return lib::BackEndLib::get()->newTexture(dataFormat,type,width,height,mipLevels,layers,sampleCount,usage);
    }

    Texture* Texture::newTexture(void* pixelData, Pixels::Format dataFormat, uint32_t width, uint32_t height, uint32_t mipLevels, TextureUsage usage, Texture::Layout initializedLayout)
    {
        return lib::BackEndLib::get()->newTexture((void**) (&pixelData), 1, width * height * Pixels::pixelBytes(dataFormat), dataFormat, Texture::Type::TEXTURE_2D, width, height, mipLevels, usage,
                                                 initializedLayout);
    }

} // slag