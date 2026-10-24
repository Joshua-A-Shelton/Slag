#include "vulkan/VkMemory.h"

namespace slag::vk {

uint32_t findMemoryType(const VkPhysicalDeviceMemoryProperties& properties, uint32_t typeBits,
                        VkMemoryPropertyFlags required, VkMemoryPropertyFlags preferred, VkMemoryPropertyFlags avoided)
{
    uint32_t best = noMemoryType;
    int bestScore = -1;
    for (uint32_t i = 0; i < properties.memoryTypeCount; ++i)
    {
        VkMemoryPropertyFlags flags = properties.memoryTypes[i].propertyFlags;
        if (!(typeBits & (1u << i)) || (flags & required) != required)
        {
            continue;
        }
        int score = ((flags & preferred) == preferred ? 2 : 0) + ((flags & avoided) == 0 ? 1 : 0);
        if (score > bestScore)
        {
            best = i;
            bestScore = score;
        }
    }
    return best;
}

MemoryFlags memoryFlags(MemoryKind kind)
{
    constexpr VkMemoryPropertyFlags hostCoherent =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    switch (kind)
    {
    case MemoryKind::GpuOnly:
        return {VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT};
    case MemoryKind::Upload:
        return {hostCoherent, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};
    case MemoryKind::Readback:
        return {hostCoherent, VK_MEMORY_PROPERTY_HOST_CACHED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};
    case MemoryKind::GpuUpload:
        return {hostCoherent | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, 0};
    }
    return {};
}

VkBufferCreateInfo bufferCreateInfo(const GraphicsDeviceImpl& device, VkDeviceSize size, VkBufferUsageFlags usage)
{
    VkBufferCreateInfo info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    info.size = size;
    info.usage = usage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    // CONCURRENT across every queue family in use: no ownership transfers (ADR 0009).
    if (device.sharingFamilyCount > 1)
    {
        info.sharingMode = VK_SHARING_MODE_CONCURRENT;
        info.queueFamilyIndexCount = device.sharingFamilyCount;
        info.pQueueFamilyIndices = device.sharingFamilies;
    }
    else
    {
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    return info;
}

bool supportsGpuUpload(const GraphicsDeviceImpl& device, uint32_t memoryType)
{
    uint32_t heapIndex = device.memoryProperties.memoryTypes[memoryType].heapIndex;
    VkDeviceSize heapSize = device.memoryProperties.memoryHeaps[heapIndex].size;
    return heapSize * 10 >= device.info.dedicatedMemoryBytes * 9;
}

namespace {

uint32_t imageTypeBits(const GraphicsDeviceImpl& device, VkFormat format, VkImageUsageFlags usage)
{
    VkImageCreateInfo image = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    image.imageType = VK_IMAGE_TYPE_2D;
    image.format = format;
    image.extent = {256, 256, 1};
    image.mipLevels = 1;
    image.arrayLayers = 1;
    image.samples = VK_SAMPLE_COUNT_1_BIT;
    image.tiling = VK_IMAGE_TILING_OPTIMAL;
    image.usage =
        usage | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    VkDeviceImageMemoryRequirements info = {VK_STRUCTURE_TYPE_DEVICE_IMAGE_MEMORY_REQUIREMENTS};
    info.pCreateInfo = &image;
    VkMemoryRequirements2 requirements = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
    device.fn.vkGetDeviceImageMemoryRequirements(device.device, &info, &requirements);
    return requirements.memoryRequirements.memoryTypeBits;
}

} // namespace

Status selectBlockMemoryTypes(GraphicsDeviceImpl& device)
{
    VkBufferCreateInfo buffer = bufferCreateInfo(device, 65536, allBufferUsage);
    VkDeviceBufferMemoryRequirements bufferInfo = {VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS};
    bufferInfo.pCreateInfo = &buffer;
    VkMemoryRequirements2 bufferRequirements = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
    device.fn.vkGetDeviceBufferMemoryRequirements(device.device, &bufferInfo, &bufferRequirements);
    uint32_t bufferBits = bufferRequirements.memoryRequirements.memoryTypeBits;

    // Representative textures: color with every usage, depth/stencil, block-compressed.
    uint32_t textureBits =
        imageTypeBits(device, VK_FORMAT_R8G8B8A8_UNORM,
                      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) &
        imageTypeBits(device, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) &
        imageTypeBits(device, VK_FORMAT_BC7_UNORM_BLOCK, 0);

    for (size_t kind = 0; kind < 4; ++kind)
    {
        MemoryFlags flags = memoryFlags(static_cast<MemoryKind>(kind));
        // Textures only go in GpuOnly blocks (ADR 0023).
        uint32_t typeBits =
            static_cast<MemoryKind>(kind) == MemoryKind::GpuOnly ? bufferBits & textureBits : bufferBits;
        device.blockMemoryTypes[kind] =
            findMemoryType(device.memoryProperties, typeBits, flags.required, flags.preferred, flags.avoided);
        if (static_cast<MemoryKind>(kind) == MemoryKind::GpuUpload)
        {
            if (device.blockMemoryTypes[kind] != noMemoryType &&
                !supportsGpuUpload(device, device.blockMemoryTypes[kind]))
            {
                device.blockMemoryTypes[kind] = noMemoryType;
            }
        }
        else if (device.blockMemoryTypes[kind] == noMemoryType)
        {
            return Status::Unsupported;
        }
    }
    return Status::Ok;
}

Status createDedicatedBuffer(GraphicsDeviceImpl& device, VkDeviceSize size, VkBufferUsageFlags usage,
                             VkMemoryPropertyFlags required, VkMemoryPropertyFlags preferred,
                             VkMemoryPropertyFlags avoided, DedicatedBuffer& out)
{
    VkBufferCreateInfo bufferInfo = bufferCreateInfo(device, size, usage);
    VkResult result = device.fn.vkCreateBuffer(device.device, &bufferInfo, nullptr, &out.buffer);
    if (result != VK_SUCCESS)
    {
        return toStatus(device, result);
    }

    VkMemoryRequirements requirements = {};
    device.fn.vkGetBufferMemoryRequirements(device.device, out.buffer, &requirements);
    out.memoryType = findMemoryType(device.memoryProperties, requirements.memoryTypeBits, required, preferred, avoided);
    if (out.memoryType == noMemoryType)
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
