#ifndef SLAG_SLAG_SWAPCHAIN_H
#define SLAG_SLAG_SWAPCHAIN_H

namespace slag
{
    class Frame;
    class GraphicsCard;
    enum class PresentMode
    {
        IMMEDIATE,
        BUFFER,
        QUEUE
    };
    enum class AlphaCompositing
    {
        IGNORE_ALPHA,
        PREMULTIPLIED,
        POSTMULTIPLIED
    };
    struct SwapChainParameters
    {
        PresentMode presentMode = PresentMode::BUFFER;
        uint32_t imageCount = 2;
        PixelFormat imageFormat = PixelFormat::B8G8R8A8_UNORM;
        AlphaCompositing alphaCompositing = AlphaCompositing::IGNORE_ALPHA;
    };
    class SwapChain
    {
    public:
        virtual ~SwapChain() = default;
        virtual Frame* next()=0;
        [[nodiscard]] virtual Frame* currentFrame()=0;
        virtual void present()=0;
        [[nodiscard]] virtual const SwapChainParameters& parameters()const=0;
        virtual GraphicsCard* graphicsCard()=0;
    };
}
#endif //SLAG_SLAG_SWAPCHAIN_H