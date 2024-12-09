#ifndef SLAG_RECTANGLE_H
#define SLAG_RECTANGLE_H

#include <cstdint>

namespace slag
{
    ///Position denoting the top left
    struct Offset
    {
        int32_t x;
        int32_t y;
    };
    ///Size of a Rectangle
    struct Extent
    {
        uint32_t width;
        uint32_t height;
    };
    ///Rectangle
    struct Rectangle
    {
        Offset offset;
        Extent extent;
    };
}
#endif //SLAG_RECTANGLE_H
