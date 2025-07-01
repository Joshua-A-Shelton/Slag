#include "SwapChain.h"
#include <slag/backends/Backend.h>
namespace slag
{
    SwapChain* SwapChain::newSwapChain(PlatformData platformData, uint32_t width, uint32_t height, PresentMode presentMode, uint8_t desiredBackbufferCount, Pixels::Format format, FrameResources*(* createResourceFunction)(uint8_t frameIndex, SwapChain* inChain))
    {
        return Backend::current()->newSwapChain(platformData,width,height,presentMode,desiredBackbufferCount,format,createResourceFunction);
    }
} // slag
