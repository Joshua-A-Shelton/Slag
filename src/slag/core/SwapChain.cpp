#include "SwapChain.h"
#include <slag/backends/Backend.h>
namespace slag
{
    SwapChain* SwapChain::newSwapChain(PlatformData platformData, uint32_t width, uint32_t height, const SwapChainDetails& details)
    {
        return Backend::current()->newSwapChain(platformData,width,height,details);
    }
} // slag
