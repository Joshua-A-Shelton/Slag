#ifndef SLAG_DIMENSIONS_H
#define SLAG_DIMENSIONS_H
#include "Pixels.h"
namespace slag
{
    struct Offset
    {
        int32_t x;
        int32_t y;
    };

    struct Offset3D
    {
        int32_t x;
        int32_t y;
        int32_t z;
    };

    struct Extent
    {
        uint32_t width;
        uint32_t height;
    };

    struct Extent3D
    {
        uint32_t width;
        uint32_t height;
        uint32_t depth;
    };

    struct Rectangle
    {
        Offset offset;
        Extent extent;
    };

    struct TextureSubresource
    {
        Pixels::AspectFlags aspectFlags;
        uint32_t mipLevel;
        uint32_t baseArrayLayer;
        uint32_t layerCount;
    };

}
#endif //SLAG_DIMENSIONS_H
