#pragma once

// Vulkan backend: entry points (one per line in BackendFunctions.inl) and the
// backend's object types.

#include "common/Backend.h"
#include "common/Objects.h"
#include "vulkan/VkLoader.h"

#include <cstdint>
#include <mutex>

namespace slag {

const BackendInfo* slagBackend_vulkan();

} // namespace slag

namespace slag::vk {

#define SLAG_BACKEND_FN(ReturnType, name, parameters) ReturnType name parameters;
#include "common/BackendFunctions.inl"
#undef SLAG_BACKEND_FN

struct InstanceImpl : InstanceCommon
{
    const GlobalFunctions* global = nullptr;
    InstanceFunctions fn;
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT messenger = VK_NULL_HANDLE;
    bool debugUtils = false;
    // Which surface_maintenance1 is enabled; the matching swapchain_maintenance1
    // can only be enabled on devices alongside it (ADR 0056).
    bool surfaceMaintenanceKHR = false;
    bool surfaceMaintenanceEXT = false;
};

// A descriptor heap: buffer memory the CPU writes descriptors into (ADR 0029).
struct DescriptorHeap
{
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    uint8_t* mapped = nullptr;   // first slot
    VkDeviceAddress address = 0; // first slot, aligned to the heap alignment
    VkDeviceSize stride = 0;     // bytes per slot
    uint32_t slots = 0;          // user-visible slots
    VkDeviceSize reservedOffset = 0;
    VkDeviceSize reservedSize = 0; // driver-reserved range after the user slots
};

constexpr uint32_t kMaxQueueFamilies = 16;

struct GraphicsDeviceImpl : GraphicsDeviceCommon
{
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties properties = {};
    VkPhysicalDeviceDescriptorHeapPropertiesEXT heapProperties = {};
    VkPhysicalDeviceMemoryProperties memoryProperties = {};
    uint32_t queueFamilyCount = 0;
    VkQueueFlags queueFamilyFlags[kMaxQueueFamilies] = {};
    uint32_t queueFamilyQueueCount[kMaxQueueFamilies] = {};

    // Valid while initialized.
    VkDevice device = VK_NULL_HANDLE;
    DeviceFunctions fn;
    DescriptorHeap resourceHeap;
    DescriptorHeap samplerHeap;
    Vector<std::mutex*> queueLocks; // one per VkQueue shared by several Slag queues (ADR 0009)
    // Distinct queue families in use; resources are CONCURRENT across them (ADR 0009).
    uint32_t sharingFamilies[3] = {};
    uint32_t sharingFamilyCount = 0;
};

struct CommandQueueImpl : CommandQueueCommon
{
    VkQueue queue = VK_NULL_HANDLE;
    uint32_t family = 0;
    std::mutex* sharedLock = nullptr; // set when the VkQueue is shared
    // Reused by submit, so submitting doesn't allocate after warm-up.
    Vector<VkSemaphoreSubmitInfo> waitInfos;
    Vector<VkSemaphoreSubmitInfo> signalInfos;
    Vector<VkCommandBufferSubmitInfo> commandBufferInfos;
};

struct PipelineImpl : PipelineCommon
{
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
};

struct CommandBufferImpl : CommandBufferCommon
{
    VkCommandPool pool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    bool bindsHeaps = false; // graphics/compute families only
    // Reused by barrier.
    Vector<VkBufferMemoryBarrier2> bufferBarriers;
    Vector<VkMemoryBarrier2> memoryBarriers;
};

struct BufferImpl : BufferCommon
{
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE; // dedicated until the M2 sub-allocator
};

struct SemaphoreImpl : SemaphoreCommon
{
    VkSemaphore semaphore = VK_NULL_HANDLE;
};

inline InstanceImpl& instanceOf(const GraphicsDeviceImpl& device)
{
    return *static_cast<InstanceImpl*>(device.instance);
}

// VkResult to Status; marks the device lost on VK_ERROR_DEVICE_LOST.
Status toStatus(GraphicsDeviceImpl& device, VkResult result);

void setObjectName(GraphicsDeviceImpl& device, VkObjectType type, uint64_t handle, const char* name);

} // namespace slag::vk
