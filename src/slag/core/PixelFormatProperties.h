#ifndef SLAG_PIXELFORMATPROPERTIES_H
#define SLAG_PIXELFORMATPROPERTIES_H

#include "Texture.h"

namespace slag
{
    struct PixelFormatProperties
    {
        ///Deals with how the image is layed out in GPU memory
        enum class Tiling
        {
            ///Format is unsuitable for Textures
            UNSUPPORTED = 0,
            ///Format is optimized by the GPU
            OPTIMIZED,
            ///Format is layed out flat in memory, some features such as mipmaps are unavailable
            LINEAR
        };
        ///Texture Tiling style
        Tiling tiling = Tiling::UNSUPPORTED;
        ///Valid Usage flags for creating a texture (individual bits, not neccesarily combination of those bits)
        Texture::UsageFlags validUsageFlags;
        ///If a texture with the format can be linearly filtered in a textures sampler (Sampler::Filter::LINEAR)
        bool linearFilteringCapable = false;
        ///If a texture with the format can be a source for blitting
        bool blitSource = false;
        ///If a texture with the format can be the destination for blitting
        bool blitDestination = false;

    };
}
#endif //SLAG_PIXELFORMATPROPERTIES_H