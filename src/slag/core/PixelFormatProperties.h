#ifndef SLAG_PIXELFORMATPROPERTIES_H
#define SLAG_PIXELFORMATPROPERTIES_H
#include "Texture.h"
namespace slag
{
    enum class TextureTiling
    {
        ///Format is unsuitable for Textures
        UNSUPPORTED = 0,
        ///Format is optimized by the GPU
        OPTIMIZED = 1,
        ///Format is laid out flat in memory, some features such as mipmaps are unavailable
        LINEAR = 2,
    };
    struct PixelFormatProperties
    {
        ///Texture Tiling style
        TextureTiling tiling = TextureTiling::UNSUPPORTED;
        ///Valid Usage flags for creating a texture
        TextureUsageFlags validUsageFlags = TextureUsageFlags::NONE;
        ///If a texture with the format can be linearly filtered in a textures sampler
        bool linearFilteringCapable = false;
        ///If a texture with the format can be a source for blitting
        bool blitSource = false;
        ///If a texture with the format can be the destination for blitting
        bool blitDestination = false;
    };
}
#endif //SLAG_PIXELFORMATPROPERTIES_H