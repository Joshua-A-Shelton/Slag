#include "Texture.h"
#include <slag/backends/Backend.h>
#include <slag/utilities/SLAG_ASSERT.h>
#include <stdexcept>

namespace slag
{
    uint32_t Texture::width(uint32_t mipLevel)
    {
        return std::max(width() >> mipLevel,static_cast<uint32_t>(1));
    }

    uint32_t Texture::height(uint32_t mipLevel)
    {
        return std::max(height() >> mipLevel,static_cast<uint32_t>(1));
    }

    uint32_t Texture::depth(uint32_t mipLevel)
    {
        return std::max(depth() >> mipLevel,static_cast<uint32_t>(1));
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
        return formatSize * width(mipLevel) * height(mipLevel) * depth(mipLevel);
    }

    Texture* Texture::newTexture(Pixels::Format texelFormat, Type type, UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, uint32_t layers, Texture::SampleCount sampleCount)
    {
        SLAG_ASSERT(Backend::current()!=nullptr);
        return Backend::current()->newTexture(texelFormat,type,usageFlags,width,height,depth,mipLevels,layers,sampleCount);
    }

    Texture* Texture::newTexture(Pixels::Format texelFormat, Type type, UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, uint32_t layers, Texture::SampleCount sampleCount, void* texelData, uint64_t texelDataLength, TextureBufferMapping* mappings, uint32_t mappingCount)
    {
        SLAG_ASSERT(Backend::current()!=nullptr);
        return Backend::current()->newTexture(texelFormat,type,usageFlags,width,height,depth,mipLevels,layers,sampleCount,texelData,texelDataLength,mappings,mappingCount);
    }


} // slag
