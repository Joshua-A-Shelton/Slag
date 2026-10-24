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

struct MemoryFlags
{
    VkMemoryPropertyFlags required;
    VkMemoryPropertyFlags preferred;
    VkMemoryPropertyFlags avoided;
};

// ADR 0019. Upload and Readback stay in system memory, like D3D12's heaps.
MemoryFlags memoryFlags(MemoryKind kind);

// Every usage, so any buffer can be used for anything (ADR 0020).
constexpr VkBufferUsageFlags allBufferUsage =
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT |
    VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
    VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

// Create info for a buffer shared by every queue family in use (ADR 0009).
VkBufferCreateInfo bufferCreateInfo(const GraphicsDeviceImpl& device, VkDeviceSize size, VkBufferUsageFlags usage);

// GpuUpload needs full resizable BAR: a CPU-visible device-local heap about
// the size of VRAM, not the 256 MB window (ADR 0019).
bool supportsGpuUpload(const GraphicsDeviceImpl& device, uint32_t memoryType);

// Picks blockMemoryTypes once the queue families are known.
Status selectBlockMemoryTypes(GraphicsDeviceImpl& device);

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
