#ifndef SLAG_RECTANGLE_H
#define SLAG_RECTANGLE_H

#include <cstdint>

namespace slag
{
    struct Offset
    {
        int32_t x;
        int32_t y;
    };
    struct Extent
    {
        uint32_t width;
        uint32_t height;
    };
    struct Rectangle
    {
        Offset offset;
        Extent extent;
    };
}

#endif //SLAG_RECTANGLE_H