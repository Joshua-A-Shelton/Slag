#include "Swapchain.h"
#include "BackEnd/Resource.h"
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

    SwapchainBuilder::SwapchainBuilder(PlatformData platformData)
    {
        _platformData = platformData;
    }

    SwapchainBuilder& SwapchainBuilder::setWidth(uint32_t width)
    {
        _width = width;
        return *this;
    }

    SwapchainBuilder& SwapchainBuilder::setHeight(uint32_t height)
    {
        _height = height;
        return *this;
    }

    SwapchainBuilder& SwapchainBuilder::setDesiredBackBuffers(uint8_t count)
    {
        if(count > 3)
        {
            count = 3;
        }
        _backBufferCount = count;
        return *this;
    }

    SwapchainBuilder& SwapchainBuilder::setVSyncEnabled(bool enabled)
    {
        _vsyncEnabled = enabled;
        return *this;
    }

    SwapchainBuilder& SwapchainBuilder::addTextureResource(std::string name, TextureResourceDescription description)
    {
        _textureDescriptions[name] = description;
        return *this;
    }

    SwapchainBuilder& SwapchainBuilder::addCommandBufferResource(std::string name)
    {
        _commandBufferNames.insert(name);
        return *this;
    }

    SwapchainBuilder& SwapchainBuilder::addUniformBufferResource(std::string name, UniformBufferResourceDescription description)
    {
        _uniformBufferDescriptions[name] = description;
        return *this;
    }

    Swapchain* SwapchainBuilder::create()
    {
        switch (SlagLib::usingBackEnd())
        {
            case VULKAN:
#ifdef SLAG_VULKAN_BACKEND
                return new vulkan::VulkanSwapchain(_platformData,_width,_height,_backBufferCount,_vsyncEnabled,_textureDescriptions,_commandBufferNames,_uniformBufferDescriptions);
#else
                return nullptr;
#endif
            case DX12:
                return nullptr;
        }
        return nullptr;
    }
}