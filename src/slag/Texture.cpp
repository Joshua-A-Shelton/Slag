#include <cassert>
#include "Texture.h"
#include "BackEnd/BackEndLib.h"

namespace slag
{

    Texture* Texture::newTexture(void* data, Pixels::Format dataFormat, uint32_t width, uint32_t height, uint32_t mipLevels, Texture::Usage usage, Texture::Layout initializedLayout)
    {
        return lib::BackEndLib::get()->newTexture(nullptr,data,dataFormat,dataFormat,width,height,mipLevels,usage,initializedLayout);
    }

    Texture* Texture::newTexture(void* data, Pixels::Format dataFormat, Pixels::Format textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, Texture::Usage usage,
                                 Texture::Layout initializedLayout)
    {
        return lib::BackEndLib::get()->newTexture(nullptr,data,dataFormat,textureFormat,width,height,mipLevels,usage,initializedLayout);
    }
} // slag