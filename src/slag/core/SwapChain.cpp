#include "SwapChain.h"
#include <slag/backends/Backend.h>
namespace slag
{
    SwapChain* SwapChain::newSwapChain(PlatformData platformData, uint32_t width, uint32_t height, PresentMode presentMode, uint8_t frameCount, Pixels::Format format, AlphaCompositing compositing, FrameResources*(* createResourceFunction)(uint8_t frameIndex, SwapChain* inChain),void (*swapchainRebuiltFunction)(SwapChain* swapChain))
    {
        return Backend::current()->newSwapChain(platformData,width,height,presentMode,frameCount,format,compositing,createResourceFunction,swapchainRebuiltFunction);
    }
} // slag
