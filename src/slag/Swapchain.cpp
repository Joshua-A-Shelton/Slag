#include "Swapchain.h"
#include "BackEnd/BackEndLib.h"
namespace slag
{
    Swapchain::Swapchain(FrameResources* (*createResourceFunction)(size_t, Swapchain*))
    {
        createResources = createResourceFunction;
    }

    Swapchain* Swapchain::newSwapchain(PlatformData platformData, uint32_t width, uint32_t height, uint8_t backBuffers, Swapchain::PresentMode mode, Pixels::Format format,FrameResources* (*createResourceFunction)(size_t frameIndex, Swapchain* inChain))
    {
        return lib::BackEndLib::get()->newSwapchain(platformData,width,height,backBuffers,mode,format,createResourceFunction);
    }

    void Swapchain::move(Swapchain& from)
    {
        std::swap(createResources,from.createResources);
    }
} // slag