#ifndef SLAG_CLEAR_H
#define SLAG_CLEAR_H

#include <cstdint>

namespace slag
{
    ///Clear color for Color textures
    union ClearColor
    {
        float floats[4];
        int32_t ints[4];
        uint32_t uints[4];
    };
    ///Clear color for Depth textures
    struct ClearDepthStencil
    {
        float depth;
        uint32_t stencil;
    };
    ///Clear value in attachment
    union ClearValue
    {
        ClearColor color;
        ClearDepthStencil depthStencil;
    };
}

#endif //SLAG_CLEAR_H
