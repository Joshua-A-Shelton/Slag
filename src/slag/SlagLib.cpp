#include "SlagLib.h"
#include <stdexcept>
#if SLAG_VULKAN_BACKEND
#include "BackEnd/Vulkan/VulkanLib.h"
#endif

namespace slag
{
    bool SLAG_VULKAN_AVAILABLE = false;
    bool SLAG_DIRECTX_AVAILABLE = false;

    BackEnd SLAG_GRAPHICS_BACKEND;

    bool initVulkan()
    {
#if SLAG_VULKAN_BACKEND
        return vulkan::VulkanLib::initialize();
#endif
        return false;
    }
    void cleanupVulkan()
    {
#if SLAG_VULKAN_BACKEND
        vulkan::VulkanLib::cleanup();
#endif
    }
    void initOpenGL()
    {

    }
    void cleanupOpenGL()
    {

    }
    bool initDirectX()
    {
        return false;
    }
    void cleanupDirectX()
    {

    }


    bool SlagLib::initialize(SlagInitDetails details)
    {
#if SLAG_VULKAN_BACKEND
        SLAG_VULKAN_AVAILABLE = true;
#endif
#if SLAG_DX12_BACKEND
        SLAG_DIRECTX_AVAILABLE = true;
#endif
        if(details.backend == VULKAN && !SLAG_VULKAN_AVAILABLE)
        {
            throw std::runtime_error("Vulkan backend not available");
        }
        else if(details.backend == DX12 && !SLAG_DIRECTX_AVAILABLE)
        {
            throw std::runtime_error("DirectX12 backend not available");
        }
        SLAG_GRAPHICS_BACKEND = details.backend;
        switch (SLAG_GRAPHICS_BACKEND)
        {
            case VULKAN:
                return initVulkan();
            case DX12:
                return initDirectX();
        }
        return false;
    }

    void SlagLib::cleanup()
    {
        switch (SLAG_GRAPHICS_BACKEND)
        {
            case VULKAN:
                cleanupVulkan();
                break;
            case DX12:
                cleanupDirectX();
                break;
        }
    }

    BackEnd SlagLib::usingBackEnd()
    {
        return SLAG_GRAPHICS_BACKEND;
    }
} // slag