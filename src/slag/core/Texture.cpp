#include "Texture.h"

namespace slag
{
    uint32_t Texture::mipWidth(uint32_t mipLevel) const
    {
        return this->width() >> mipLevel;
    }

    uint32_t Texture::mipHeight(uint32_t mipLevel) const
    {
        return this->height() >> mipLevel;
    }

    uint32_t Texture::mipDepth(uint32_t mipLevel) const
    {
        return this->depth() >> mipLevel;
    }
} // slag
