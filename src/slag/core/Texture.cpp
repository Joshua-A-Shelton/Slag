#include "Texture.h"
#include <slag/backends/Backend.h>
#include <slag/utilities/SLAG_ASSERT.h>
#include <stdexcept>

namespace slag
{
    uint32_t Texture::width(uint32_t mipLevel)
    {
        return width() >> mipLevel;
    }

    uint32_t Texture::height(uint32_t mipLevel)
    {
        return height() >> mipLevel;
    }

    uint64_t Texture::byteSize()
    {

        auto mipLevels = this->mipLevels();
        uint64_t bytes = 0;
        for (uint32_t i = 0; i < mipLevels; i++)
        {
            bytes += byteSize(i);
        }
        bytes*=layers();
        return bytes;
    }

    uint64_t Texture::byteSize(uint32_t mipLevel)
    {
        auto format = this->format();
        auto formatSize = Pixels::size(format);
        return formatSize * width(mipLevel) * height(mipLevel);
    }

    Texture* Texture::newTexture(Pixels::Format texelFormat, Type type, UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t layers, uint32_t mipLevels)
    {
        SLAG_ASSERT(Backend::current()!=nullptr);
        return Backend::current()->newTexture(texelFormat,type,usageFlags,width,height,layers,mipLevels);
    }

    Texture* Texture::newTexture(Pixels::Format texelFormat, Type type, UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t layers, uint32_t mipLevels, void* texelData, uint32_t providedDataMips, uint32_t providedDataLayers)
    {
        SLAG_ASSERT(Backend::current()!=nullptr);
        return Backend::current()->newTexture(texelFormat,type,usageFlags,width,height,layers,mipLevels,texelData,providedDataMips,providedDataLayers);
    }


} // slag
