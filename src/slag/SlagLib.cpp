#include "SlagLib.h"
#include "BackEnd/BackEndLib.h"
#if SLAG_VULKAN_BACKEND
#include "BackEnd/Vulkan/VulkanLib.h"
#endif
#if SLAG_DX12_BACKEND
#include "BackEnd/DX12/DX12Lib.h"
#endif
namespace slag
{

    BackEnd _usingBackend = Vulkan;

    bool SlagLib::initialize(const SlagInitDetails& details)
    {

        _usingBackend = details.backend;
        switch (_usingBackend)
        {
            case BackEnd::Vulkan:
#if SLAG_VULKAN_BACKEND
                lib::BackEndLib::set(vulkan::VulkanLib::initialize(details));
#endif
                break;
            case BackEnd::DirectX12:
#if SLAG_DX12_BACKEND
                lib::BackEndLib::set(dx::DX12Lib::initialize(details));
#endif
                break;
        }
        return lib::BackEndLib::get()!=nullptr;
    }

    void SlagLib::cleanup()
    {
        switch (_usingBackend)
        {
            case BackEnd::Vulkan:
#if SLAG_VULKAN_BACKEND
                vulkan::VulkanLib::cleanup(lib::BackEndLib::get());
#endif
                break;
            case BackEnd::DirectX12:
#if SLAG_DX12_BACKEND
                dx::DX12Lib::cleanup(lib::BackEndLib::get());
#endif
                break;
        }
        lib::BackEndLib::set(nullptr);
    }

    BackEnd SlagLib::usingBackEnd()
    {
        return lib::BackEndLib::get()->identifier();
    }

    GraphicsCard* SlagLib::graphicsCard()
    {
        return lib::BackEndLib::get()->graphicsCard();
    }
} // slag