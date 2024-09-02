#ifndef SLAG_GPUMEMORYBARRIERS_H
#define SLAG_GPUMEMORYBARRIERS_H

#include "Texture.h"

namespace slag
{
    struct ImageBarrier
    {
        Texture* texture;
        Texture::Layout oldLayout;
        Texture::Layout newLayout;
    };

    struct BufferBarrier
    {

    };
}

#endif //SLAG_GPUMEMORYBARRIERS_H
