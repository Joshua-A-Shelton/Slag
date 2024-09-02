#include "Swapchain.h"
#include "BackEnd/BackEndLib.h"
namespace slag
{
    Swapchain*
    Swapchain::newSwapchain(PlatformData platformData, uint32_t width, uint32_t height, uint8_t backBuffers, Swapchain::PresentMode mode, Pixels::Format format)
    {
        return lib::BackEndLib::get()->newSwapchain(platformData,width,height,backBuffers,mode,format);
    }
} // slag