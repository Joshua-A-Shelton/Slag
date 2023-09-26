#ifndef SLAG_SWAPCHAIN_H
#define SLAG_SWAPCHAIN_H
#include "Frame.h"

namespace slag
{
    class Resource;
    class Swapchain
    {
    public:
        Swapchain();
        virtual ~Swapchain();
        virtual Frame* currentFrame()=0;
        virtual Frame* next()=0;
        friend class Resource;
    private:

    };
}
#endif //SLAG_SWAPCHAIN_H