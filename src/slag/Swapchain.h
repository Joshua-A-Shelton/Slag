#ifndef SLAG_SWAPCHAIN_H
#define SLAG_SWAPCHAIN_H

#include <cstdint>
#include "Frame.h"
#include "PlatformData.h"
#include "Pixel.h"

namespace slag
{

    class Swapchain
    {
    public:
        enum PresentMode
        {
            MAILBOX,
            FIFO
        };
        virtual ~Swapchain()=default;
        virtual Frame* next() = 0;
        virtual Frame* nextIfReady()=0;
        virtual Frame* currentFrame() = 0;
        virtual uint8_t currentFrameIndex() = 0;
        virtual uint8_t backBuffers() = 0;
        virtual void backBuffers(uint8_t count) = 0;
        virtual uint32_t width() = 0;
        virtual uint32_t height() = 0;
        virtual void resize(uint32_t width, uint32_t height) = 0;
        virtual PresentMode presentMode()=0;
        virtual void presentMode(PresentMode mode)=0;

        static Swapchain* newSwapchain(PlatformData platformData,uint32_t width, uint32_t height, uint8_t backBuffers, PresentMode mode, Pixels::Format format);
    };

} // slag

#endif //SLAG_SWAPCHAIN_H
