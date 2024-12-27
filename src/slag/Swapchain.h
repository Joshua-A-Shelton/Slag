#ifndef SLAG_SWAPCHAIN_H
#define SLAG_SWAPCHAIN_H

#include <cstdint>
#include "Frame.h"
#include "PlatformData.h"
#include "Pixel.h"

namespace slag
{
    ///Links platform window to rendering, and how to swap between back buffers if they're present
    class Swapchain
    {
    public:
        ///Ordering of back buffers
        enum PresentMode
        {
            ///Latest submitted back buffer is the one shown when presented to window
            MAILBOX,
            ///First back buffer in queue is the one shown when presented to window, and advances the queue
            FIFO
        };
        Swapchain(FrameResources* (*createResourceFunction)(size_t frameIndex, Swapchain* inChain));
        virtual ~Swapchain()=default;
        ///Acquire the next frame, blocks until the frame is ready, or null if there's none to acquire (usually minimized window)
        virtual Frame* next() = 0;
        ///Acquire the next frame if the next frame is finished with its operations, or null if it's still performing operations or there's none to acquire (usually minimized window)
        virtual Frame* nextIfReady()=0;
        ///The current frame
        virtual Frame* currentFrame() = 0;
        ///The index of the current frame
        virtual uint8_t currentFrameIndex() = 0;
        ///The number of backbuffers we have to render to
        virtual uint8_t backBuffers() = 0;
        /**
         * Sets the number of backbuffers the swapchain has
         * @param count the new number of backbuffers this swapchain has
         */
        virtual void backBuffers(uint8_t count) = 0;
        ///Current width of the backbuffers in the swapchain
        virtual uint32_t width() = 0;
        ///Current height of the backbuffers in the swapchain
        virtual uint32_t height() = 0;
        ///Resizes the backbuffers in the swapchain to a new size
        virtual void resize(uint32_t width, uint32_t height) = 0;
        ///Current present mode
        virtual PresentMode presentMode()=0;
        ///Set the swapchain to have given present mode
        virtual void presentMode(PresentMode mode)=0;
        /**
         * Creates a new swapchain
         * @param platformData Platform dependent windowing information
         * @param width Width in pixels of backbuffers in chain
         * @param height Height in pixels of backbuffers in chain
         * @param backBuffers Number of backbuffers in chain
         * @param mode Swap Operation of chain
         * @param format Texture format for backbuffers in chain
         * @return
         */
        static Swapchain* newSwapchain(PlatformData platformData,uint32_t width, uint32_t height, uint8_t backBuffers, PresentMode mode, Pixels::Format format, FrameResources* (*createResourceFunction)(size_t frameIndex, Swapchain* inChain)=nullptr);

    protected:
        void move(Swapchain& from);
        FrameResources* (*createResources)(size_t frameIndex, Swapchain* inChain) = nullptr;
    };

} // slag

#endif //SLAG_SWAPCHAIN_H
