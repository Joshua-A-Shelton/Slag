#ifndef SLAG_SLAG_SWAPCHAIN_H
#define SLAG_SLAG_SWAPCHAIN_H

namespace slag
{
    class Frame;
    class GraphicsCard;
    ///Controls how the back buffer is presented to the screen.
    enum class PresentMode
    {
        ///When the frame is presented, show immediately, regardless of vertical blank (No VSYNC)
        IMMEDIATE,
        ///When the frame is presented, replace the last frame presented and show the most current frame at VSYNC
        BUFFER,
        ///When the frame is presented, append the frame to a queue and show the first frame in the queue at VSYNC
        QUEUE
    };
    ///How to integrate alpha values in the final frame with the rest of the display environment
    enum class AlphaCompositing
    {
        ///Alpha values in the final frame are treated as opaque
        IGNORE_ALPHA,
        ///RGB channels are multiplied by alpha value before compositing
        PREMULTIPLIED,
        ///RGB channels are multiplied by alpha value after compositing
        POSTMULTIPLIED
    };
    ///Parameters that control the details of a swapchain
    struct SwapChainParameters
    {
        ///Controls how the back buffer is presented to the screen.
        PresentMode presentMode = PresentMode::BUFFER;
        ///Number of images in the swapchain
        uint32_t imageCount = 2;
        ///Format of the images in the swapchain
        PixelFormat imageFormat = PixelFormat::B8G8R8A8_UNORM;
        ///How to integrate alpha values in the final frame with the rest of the display environment
        AlphaCompositing alphaCompositing = AlphaCompositing::IGNORE_ALPHA;
    };
    ///Mechanism that deals with acquiring and presenting images to the screen
    class SwapChain
    {
    public:
        virtual ~SwapChain() = default;
        virtual Frame* next()=0;
        [[nodiscard]] virtual Frame* currentFrame()=0;
        virtual void present()=0;
        [[nodiscard]] virtual const SwapChainParameters& parameters()const=0;
        virtual void setParameters(const SwapChainParameters& newParameters)=0;
        virtual GraphicsCard* graphicsCard()=0;
    };
}
#endif //SLAG_SLAG_SWAPCHAIN_H