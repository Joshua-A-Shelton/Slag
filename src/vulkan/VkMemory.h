#pragma once

// Memory type selection and raw allocations for the Vulkan backend.

#include "vulkan/VkBackend.h"

namespace slag::vk {

constexpr uint32_t noMemoryType = UINT32_MAX;

// Picks a type allowed by `typeBits` with every `required` flag. Among those,
// prefers one with every `preferred` flag, then one with no `avoided` flag.
uint32_t findMemoryType(const VkPhysicalDeviceMemoryProperties& properties, uint32_t typeBits,
                        VkMemoryPropertyFlags required, VkMemoryPropertyFlags preferred,
                        VkMemoryPropertyFlags avoided = 0);

// A buffer with its own memory, optionally mapped.
struct DedicatedBuffer
{
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void* mapped = nullptr;
    VkDeviceAddress address = 0;
    uint32_t memoryType = noMemoryType;
};

Status createDedicatedBuffer(GraphicsDeviceImpl& device, VkDeviceSize size, VkBufferUsageFlags usage,
                             VkMemoryPropertyFlags required, VkMemoryPropertyFlags preferred,
                             VkMemoryPropertyFlags avoided, DedicatedBuffer& out);
void destroyDedicatedBuffer(GraphicsDeviceImpl& device, DedicatedBuffer& buffer);

} // namespace slag::vk
