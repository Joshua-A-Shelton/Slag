#ifndef SLAG_GPUMEMORYBARRIER_H
#define SLAG_GPUMEMORYBARRIER_H
#include "Texture.h"
namespace slag
{
    enum MemoryAccess
    {

    };
    struct ImageMemoryBarrier
    {
        MemoryAccess source;
        MemoryAccess destination;
        Texture::Layout oldLayout = Texture::Layout::UNDEFINED;
        Texture::Layout newLayout = Texture::Layout::UNDEFINED;
    };
}
#endif //SLAG_GPUMEMORYBARRIER_H
