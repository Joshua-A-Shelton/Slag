#include "Texture.h"

#include <algorithm>

#include "slag/utilities/SLAG_ASSERT.h"

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

    uint64_t Texture::bufferSize(PixelAspect aspect) const
    {
        uint64_t totalSize = 0;
        auto aspectSize = Pixel::aspectSize(this->format(),aspect);
        for (uint32_t layers = 0; layers < this->layers(); layers++)
        {
            for (uint32_t mipLevel=0; mipLevel< this->mipLevels(); mipLevel++)
            {
                totalSize += static_cast<uint64_t>(aspectSize) * mipWidth(mipLevel) * std::max(mipHeight(mipLevel),1u) * std::max(mipDepth(mipLevel), 1u);
            }
        }

        return totalSize;
    }

} // slag
