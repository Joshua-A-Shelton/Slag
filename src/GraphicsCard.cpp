#include "GraphicsCard.h"
#include <VkBootstrap.h>
#include <stdexcept>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include "BackEnd/VulkanBackEnd.h"

namespace slag
{
    GraphicsCard::GraphicsCard(void *vkbDevice)
    {
        vkb::Device* vkb_device = reinterpret_cast<vkb::Device *>(vkbDevice);
        _vkDevice = vkb_device->device;
        _vkPhysicalDevice = vkb_device->physical_device;
        auto graphicsQueue_ret = vkb_device->get_queue(vkb::QueueType::graphics);
        if(!graphicsQueue_ret)
        {
            throw std::runtime_error("Failed to get graphics queue");
        }
        _vkGraphicsQueue = graphicsQueue_ret.value();
        _graphicsQueueFamily = vkb_device->get_queue_index(vkb::QueueType::graphics).value();

        auto presentQueue_ret = vkb_device->get_queue(vkb::QueueType::present);
        if(!presentQueue_ret)
        {
            _vkPresentQueue = _vkGraphicsQueue;
        }
        else
        {
            _vkPresentQueue = presentQueue_ret.value();
        }

        auto computeQueue_ret = vkb_device->get_queue(vkb::QueueType::compute);
        if(!computeQueue_ret)
        {
            _hasComputeCapabilities = false;
        }
        else
        {
            _hasComputeCapabilities = true;
            _vkComputeQueue = computeQueue_ret.value();
        }

        VmaAllocator allocator;
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = vkb_device->physical_device;
        allocatorInfo.device = vkb_device->device;
        allocatorInfo.instance = slag::_vulkanInstance();
        if(vmaCreateAllocator(&allocatorInfo, &allocator)!= VK_SUCCESS)
        {
            throw std::runtime_error("Unable to create memory allocator");
        }
        _vmaAllocator = allocator;

    }

    bool GraphicsCard::hasComputeCapabilities()
    {
        return _hasComputeCapabilities;
    }

    GraphicsCard::~GraphicsCard()
    {
        vmaDestroyAllocator(static_cast<VmaAllocator>(_vmaAllocator));
        vkDestroyDevice(static_cast<VkDevice>(_vkDevice), nullptr);
    }
}