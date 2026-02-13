#include "Slag.h"
#include <slag/core/IBackend.h>
#ifdef SLAG_VULKAN_BACKEND
#include <slag/backends/vulkan/VulkanBackend.h>
#endif
#ifdef SLAG_DX12_BACKEND
#include <slag/backends/dx12/DX12Backend.h>
#endif
namespace slag
{
    IBackend* SLAG_GRAPHICS_BACKEND = nullptr;
    SlagInitializationResult Slag::initialize(const InitializationData& initData)
    {
        auto desiredBackend = initData.backend;
        if (desiredBackend == BackendAPI::UNKNOWN)
        {
#ifdef SLAG_DX12_BACKEND
            desiredBackend = BackendAPI::DX12;
#else
            desiredBackend = BackendAPI::VULKAN;
#endif

        }

        switch (desiredBackend)
        {
#ifdef SLAG_VULKAN_BACKEND
        case BackendAPI::VULKAN:
            SLAG_GRAPHICS_BACKEND = new vulkan::VulkanBackend();
            break;
#endif
#ifdef SLAG_DX12_BACKEND
        case BackendAPI::DX12:
            SLAG_GRAPHICS_BACKEND = new dx12::DX12Backend();
            break;
#endif
        case BackendAPI::CUSTOM:
            if (initData.customBackend!= nullptr)
            {
                SLAG_GRAPHICS_BACKEND = initData.customBackend;
            }
            else
            {
                return SlagInitializationResult::BACKEND_UNSUPPORTED;
            }
            break;
        default:
            return SlagInitializationResult::BACKEND_UNSUPPORTED;
            break;
        }

        auto result = SLAG_GRAPHICS_BACKEND->initializeBackend(initData);
        if (result != SlagInitializationResult::SUCCESS)
        {
            delete SLAG_GRAPHICS_BACKEND;
            SLAG_GRAPHICS_BACKEND = nullptr;
            return result;
        }
        if (SLAG_GRAPHICS_BACKEND->graphicsCardCount() == 0)
        {
            delete SLAG_GRAPHICS_BACKEND;
            SLAG_GRAPHICS_BACKEND = nullptr;
            return SlagInitializationResult::NO_GRAPHICS_CARDS;
        }
        return SlagInitializationResult::SUCCESS;
    }

    void Slag::cleanup()
    {
        if (SLAG_GRAPHICS_BACKEND != nullptr)
        {
            delete SLAG_GRAPHICS_BACKEND;
            SLAG_GRAPHICS_BACKEND = nullptr;
        }
    }

    IBackend* Slag::backend()
    {
        return SLAG_GRAPHICS_BACKEND;
    }
} // slag
