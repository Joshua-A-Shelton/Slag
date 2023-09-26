#include "Swapchain.h"
#include "Resource.h"
#include "BackEnd/ResourceManager.h"
#include "SlagLib.h"
#include "BackEnd/Vulkan/VulkanSwapchain.h"

namespace slag
{

    Swapchain::Swapchain()
    {
        backend::ResourceManager::addSwapchain(this);
    }

    Swapchain::~Swapchain()
    {
        backend::ResourceManager::removeSwapchain(this);
    }

    Swapchain* Swapchain::create(PlatformData platformData, uint32_t width, uint32_t height, size_t desiredBackbuffers, bool vsync)
    {
        switch (SlagLib::usingBackEnd())
        {
            case VULKAN:
                return new vulkan::VulkanSwapchain(platformData,width,height,desiredBackbuffers,vsync);
            case DX12:
                return nullptr;
        }
        return nullptr;
    }

}