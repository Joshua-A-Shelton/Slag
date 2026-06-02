#ifndef SLAG_SLAG_SWAPCHAIN_H
#define SLAG_SLAG_SWAPCHAIN_H

namespace slag
{
    class Frame;
    enum class PresentMode
    {
        IMMEDIATE,
        BUFFER,
        QUEUE
    };
    class SwapChain
    {
    public:
        virtual ~SwapChain() = default;
        virtual Frame* next()=0;
        virtual Frame* nextIfReady()=0;
        virtual GraphicsCard* graphicsCard()=0;
    };
}
#endif //SLAG_SLAG_SWAPCHAIN_H