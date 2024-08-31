#ifndef SLAG_SWAPCHAIN_H
#define SLAG_SWAPCHAIN_H

#include "Frame.h"

namespace slag
{

    class Swapchain
    {
    public:
        enum PresentMode
        {
            Discard,
            Sequential
        };
        virtual ~Swapchain()=default;
        virtual Frame* next() = 0;
        virtual Frame* currentFrame() = 0;
        virtual uint8_t currentFrameIndex() = 0;
        virtual bool vsync() = 0;
        virtual void vsync(bool enable) = 0;
        virtual uint8_t backBuffers() = 0;
        virtual void backBuffers(uint8_t count) = 0;
        virtual uint32_t width() = 0;
        virtual uint32_t height() = 0;
        virtual void resize(uint32_t width, uint32_t height) = 0;
        virtual PresentMode presentMode()=0;
        virtual void presentMode(PresentMode mode)=0;

        static Swapchain* newSwapchain(uint32_t width, uint32_t height, uint8_t backBuffers, bool vsync, PresentMode mode);
    };

} // slag

#endif //SLAG_SWAPCHAIN_H
