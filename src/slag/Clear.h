#ifndef SLAG_CLEAR_H
#define SLAG_CLEAR_H

#include <cstdint>

namespace slag
{
    union ClearColor
    {
        float floats[4];
        int32_t ints[4];
        uint32_t uints[4];
    };

    struct ClearDepthStencil
    {
        float depth;
        uint32_t stencil;
    };

    union ClearValue
    {
        ClearColor color;
        ClearDepthStencil depthStencil;
    };
}

#endif //SLAG_CLEAR_H
