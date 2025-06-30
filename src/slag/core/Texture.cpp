#include "Texture.h"

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

    Texture* Texture::newTexture(Pixels::Format texelFormat, Type type, UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t layers, uint32_t mipLevels)
    {
        throw std::runtime_error("Texture::newTexture(): Not implemented");
    }

    Texture* Texture::newTexture(Pixels::Format texelFormat, Type type, UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t layers, uint32_t mipLevels, void* texelData, uint32_t providedDataMips, uint32_t providedDataLayers)
    {
        throw std::runtime_error("Texture::newTexture(): Not implemented");
    }


} // slag
