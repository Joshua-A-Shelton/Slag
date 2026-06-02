#ifndef SLAG_SLAG_FRAME_H
#define SLAG_SLAG_FRAME_H
#include "Texture.h"
namespace slag
{
    class Frame
    {
    public:
        virtual ~Frame() = default;
        [[nodiscard]] virtual Texture* renderBuffer()=0;

    };
}
#endif //SLAG_SLAG_FRAME_H