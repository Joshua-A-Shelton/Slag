#ifndef SLAG_SWAPCHAIN_H
#define SLAG_SWAPCHAIN_H
#include "Frame.h"
#include "PlatformData.h"

namespace slag
{
    class Resource;
    class Swapchain
    {
    public:
        static Swapchain* create(PlatformData platformData, uint32_t width, uint32_t height, size_t desiredBackbuffers, bool vsync);
        Swapchain();
        virtual ~Swapchain();
        virtual Frame* currentFrame()=0;
        virtual Frame* next()=0;
        virtual size_t backBufferCount()=0;
        virtual void backBufferCount(size_t count)=0;
        virtual bool vsyncEnabled()=0;
        virtual void vsyncEnabled(bool enabled)=0;
        virtual uint32_t width()=0;
        virtual uint32_t height()=0;
        virtual void resize(uint32_t width, uint32_t height)=0;
    private:

    };
}
#endif //SLAG_SWAPCHAIN_H