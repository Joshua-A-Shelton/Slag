#include "vulkan/VkMemory.h"

namespace slag::vk {

uint32_t findMemoryType(const VkPhysicalDeviceMemoryProperties& properties, uint32_t typeBits,
                        VkMemoryPropertyFlags required, VkMemoryPropertyFlags preferred)
{
    uint32_t fallback = kNoMemoryType;
    for (uint32_t i = 0; i < properties.memoryTypeCount; ++i)
    {
        if (!(typeBits & (1u << i)))
        {
            continue;
        }
        VkMemoryPropertyFlags flags = properties.memoryTypes[i].propertyFlags;
        if ((flags & required) != required)
        {
            continue;
        }
        if ((flags & preferred) == preferred)
        {
            return i;
        }
        if (fallback == kNoMemoryType)
        {
            fallback = i;
        }
    }
    return fallback;
}

Status createDedicatedBuffer(GraphicsDeviceImpl& device, VkDeviceSize size, VkBufferUsageFlags usage,
                             VkMemoryPropertyFlags required, VkMemoryPropertyFlags preferred, DedicatedBuffer& out)
{
    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = size;
    bufferInfo.usage = usage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkResult result = device.fn.vkCreateBuffer(device.device, &bufferInfo, nullptr, &out.buffer);
    if (result != VK_SUCCESS)
    {
        return toStatus(device, result);
    }

    VkMemoryRequirements requirements = {};
    device.fn.vkGetBufferMemoryRequirements(device.device, out.buffer, &requirements);
    out.memoryType = findMemoryType(device.memoryProperties, requirements.memoryTypeBits, required, preferred);
    if (out.memoryType == kNoMemoryType)
    {
        destroyDedicatedBuffer(device, out);
        return Status::Unsupported;
    }

    VkMemoryAllocateFlagsInfo flags = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};
    flags.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
    VkMemoryAllocateInfo allocateInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocateInfo.pNext = &flags;
    allocateInfo.allocationSize = requirements.size;
    allocateInfo.memoryTypeIndex = out.memoryType;
    result = device.fn.vkAllocateMemory(device.device, &allocateInfo, nullptr, &out.memory);
    if (result != VK_SUCCESS)
    {
        destroyDedicatedBuffer(device, out);
        return toStatus(device, result);
    }
    result = device.fn.vkBindBufferMemory(device.device, out.buffer, out.memory, 0);
    if (result != VK_SUCCESS)
    {
        destroyDedicatedBuffer(device, out);
        return toStatus(device, result);
    }

    VkMemoryPropertyFlags typeFlags = device.memoryProperties.memoryTypes[out.memoryType].propertyFlags;
    if (typeFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        result = device.fn.vkMapMemory(device.device, out.memory, 0, VK_WHOLE_SIZE, 0, &out.mapped);
        if (result != VK_SUCCESS)
        {
            destroyDedicatedBuffer(device, out);
            return toStatus(device, result);
        }
    }

    VkBufferDeviceAddressInfo addressInfo = {VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    addressInfo.buffer = out.buffer;
    out.address = device.fn.vkGetBufferDeviceAddress(device.device, &addressInfo);
    return Status::Ok;
}

void destroyDedicatedBuffer(GraphicsDeviceImpl& device, DedicatedBuffer& buffer)
{
    if (buffer.mapped)
    {
        device.fn.vkUnmapMemory(device.device, buffer.memory);
    }
    if (buffer.buffer != VK_NULL_HANDLE)
    {
        device.fn.vkDestroyBuffer(device.device, buffer.buffer, nullptr);
    }
    if (buffer.memory != VK_NULL_HANDLE)
    {
        device.fn.vkFreeMemory(device.device, buffer.memory, nullptr);
    }
    buffer = {};
}

} // namespace slag::vk
