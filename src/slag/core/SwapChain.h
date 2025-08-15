#ifndef SLAG_SWAPCHAIN_H
#define SLAG_SWAPCHAIN_H
//SlagValue, VulkanValue, DX12Value, AllowTearing, FrameLatency
#define SLAG_PRESENT_MODE_DEFINTITIONS(DEFINITION) \
DEFINITION(IMMEDIATE,VK_PRESENT_MODE_IMMEDIATE_KHR,DXGI_SWAP_EFFECT_FLIP_DISCARD,true,1)\
DEFINITION(BUFFER,VK_PRESENT_MODE_MAILBOX_KHR,DXGI_SWAP_EFFECT_FLIP_DISCARD,false,1)\
DEFINITION(QUEUE,VK_PRESENT_MODE_FIFO_KHR,DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL,false,1)\

#define SLAG_SWAPCHAIN_ALPHA_MODE(DEFINITION)\
DEFINITION(IGNORE_ALPHA, VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,DXGI_ALPHA_MODE_IGNORE)\
DEFINITION(PRE_MULTIPLY, VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,DXGI_ALPHA_MODE_PREMULTIPLIED)\
DEFINITION(POST_MULTIPLY, VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,DXGI_ALPHA_MODE_STRAIGHT)\



#include "Pixels.h"
#include "PlatformData.h"

namespace slag
{
    class Frame;
    class FrameResources;
    ///Links platform window to rendering, and how to swap between back buffers if they're present
    class SwapChain
    {
    public:
        enum class PresentMode
        {
#define DEFINITION(SlagValue, VulkanValue, DX12Value, AllowTearing, FrameLatency) SlagValue,
            SLAG_PRESENT_MODE_DEFINTITIONS(DEFINITION)
#undef DEFINITION
        };
        enum class AlphaCompositing
        {
#define DEFINITION(SlagValue, VulkanValue, DX12Value) SlagValue,
            SLAG_SWAPCHAIN_ALPHA_MODE(DEFINITION)
#undef DEFINITION
        };
        virtual ~SwapChain()=default;
        ///Acquire the next frame, blocks until the frame is ready, or null if there's none to acquire (usually minimized window)
        [[nodiscard]]virtual Frame* next()=0;
        ///Acquire the next frame if the next frame is finished with its operations, or null if it's still performing operations or there's none to acquire (usually minimized window)
        [[nodiscard]]virtual Frame* nextIfReady()=0;
        ///The current frame
        virtual Frame* currentFrame()=0;
        ///The number of frames that can be rendered to
        virtual uint8_t framesCount()=0;
        ///The index of the current frame
        virtual uint8_t currentFrameIndex()=0;

        ///Texel format of the backbuffers
        virtual Pixels::Format backBufferFormat()=0;

        /**
         * Set the Texel format of the backbuffers
         * @param newFormat New desired format for the swapchain backbuffers
         */
        virtual void backBufferFormat(Pixels::Format newFormat)=0;
        ///Width in texels of the backbuffers
        virtual uint32_t backBufferWidth()=0;
        ///Height in texels of the backbuffers
        virtual uint32_t backBufferHeight()=0;

        /**
         * Resize the backbuffers
         * @param newWidth Desired width of the backbuffers
         * @param newHeight Desired height of the backbuffers
         */
        virtual void backBufferSize(uint32_t newWidth, uint32_t newHeight)=0;
        ///Present mode the backbuffer is using
        virtual PresentMode presentMode()=0;
        ///number of frames the chain has
        virtual uint8_t frameCount()=0;

        /**
         * Set the present mode
         * @param newMode Desired present mode
         * @param frameCount Desired frame count
         */
        virtual void presentMode(PresentMode newMode, uint8_t frameCount)=0;
        ///Compositing mode for blending the backbuffer images into the host environment
        virtual AlphaCompositing alphaCompositing()=0;
        /**
         * Set the compositing mode
         * @param newCompositing Desired compositing rule
         */
        virtual void alphaComposition(AlphaCompositing newCompositing)=0;

        /**
         * Set all swapchain properties at once, prevents rebuilding the swapchain for each individual parameter
         * @param newFormat New pixel format
         * @param newWidth New backbuffer width
         * @param newHeight New backbuffer height
         * @param newMode New chain presentation mode
         * @param frames New frame count
         * @param compositing New compositing rule
         */
        virtual void setProperties(uint32_t newWidth, uint32_t newHeight, uint8_t frames,PresentMode newMode, Pixels::Format newFormat, AlphaCompositing compositing)=0;

        /**
         * Creates a new swapchain
         * @param platformData Platform dependent windowing information
         * @param width Width in pixels of backbuffers in chain
         * @param height Height in pixels of backbuffers in chain
         * @param presentMode Swap Operation of chain
         * @param compositing How the images of this swapchain are blended into the native environment
         * @param desiredBackbufferCount Number of backbuffers in chain
         * @param format Texture format for backbuffers in chain
         * @param createResourceFunction Optional function to provide extra data to each frame
         * @return 
         */
        static SwapChain* newSwapChain(PlatformData platformData, uint32_t width, uint32_t height, PresentMode presentMode, uint8_t frameCount,Pixels::Format format,AlphaCompositing compositing, FrameResources* (*createResourceFunction)(uint8_t frameIndex, SwapChain* inChain)=nullptr, void (*swapchainRebuiltFunction)(SwapChain* swapChain)=nullptr);

    };
} // slag

#endif //SLAG_SWAPCHAIN_H
