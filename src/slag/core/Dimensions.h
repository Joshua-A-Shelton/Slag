#ifndef SLAG_DIMENSIONS_H
#define SLAG_DIMENSIONS_H
#include <cstdint>

#include "Pixels.h"
namespace slag
{
    struct Offset2D
    {
        int32_t x=0;
        int32_t y=0;
    };

    struct Offset3D
    {
        int32_t x=0;
        int32_t y=0;
        int32_t z=0;
    };

    struct Extent2D
    {
        uint32_t width=0;
        uint32_t height=0;
    };

    struct Extent3D
    {
        uint32_t width=0;
        uint32_t height=0;
        uint32_t depth=0;
    };

    struct Rectangle
    {
        Offset2D offset{};
        Extent2D extent{};
    };

    struct TextureSubresource
    {
        PixelAspect aspect = PixelAspect::COLOR;
        uint32_t mipLevel = 0;
        uint32_t baseArrayLayer = 0;
        uint32_t layerCount = 1;
    };

    struct TextureBufferMapping
    {
        uint64_t bufferOffset = 0;
        TextureSubresource subresource{};
        Offset3D offset{};
        Extent3D extent{};
    };

}
#endif //SLAG_DIMENSIONS_H