#ifndef SLAG_TEXTURE_H
#define SLAG_TEXTURE_H
#include <cstdint>
#include "Resource.h"
#include "PixelFormat.h"

namespace slag
{
    class Texture: public Resource
    {

    public:
        enum Usage
        {
            COLOR = 0x00000001,
            DEPTH = 0x00000002,
            STENCIL = 0x00000004,
            DEPTH_STENCIL = DEPTH | STENCIL
        };
        virtual ~Texture()=default;

        virtual PixelFormat format()=0;
        virtual uint32_t mipLevels()=0;
        virtual uint32_t width()=0;
        virtual uint32_t height()=0;
        virtual Usage usage()=0;
    };
}
#endif //SLAG_TEXTURE_H