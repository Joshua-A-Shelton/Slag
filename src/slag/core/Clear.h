#ifndef SLAG_CLEAR_H
#define SLAG_CLEAR_H
#include <cstdint>

namespace slag
{
    ///Clear color for Color textures
    union ClearColor
    {
        float floats[4]{0,0,0,0};
        int32_t ints[4]{0,0,0,0};
        uint32_t uints[4]{0,0,0,0};
    };

    ///Clear color for Depth textures
    struct ClearDepthStencilValue
    {
        float depth{0};
        uint32_t stencil{0};
    };
    ///Clear value in attachment
    union ClearValue
    {
        ClearColor color;
        ClearDepthStencilValue depthStencil;
    };

}
#endif //SLAG_CLEAR_H
