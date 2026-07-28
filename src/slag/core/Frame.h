#ifndef SLAG_SLAG_FRAME_H
#define SLAG_SLAG_FRAME_H
#include "Texture.h"
namespace slag
{
    class Frame
    {
    public:
        virtual ~Frame() = default;
        ///The texture that is presented when SwapChain::present is called
        [[nodiscard]] virtual Texture* renderBuffer()=0;

    };
}
#endif //SLAG_SLAG_FRAME_H