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

constexpr uint32_t maxQueueFamilies = 16;

// Push data layout: push constants, then the root constant buffer's address
// (ADRs 0003, 0044).
constexpr uint32_t constantBufferPushOffset = 128;

// Maps the root constant buffer (set 0, binding 1) to its push address; chain
// into every shader stage's create info.
VkShaderDescriptorSetAndBindingMappingInfoEXT constantBufferMapping();

struct GraphicsDeviceImpl : GraphicsDeviceCommon
{
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties properties = {};
    VkPhysicalDeviceDescriptorHeapPropertiesEXT heapProperties = {};
    VkPhysicalDeviceMemoryProperties memoryProperties = {};
    uint32_t queueFamilyCount = 0;
    VkQueueFlags queueFamilyFlags[maxQueueFamilies] = {};
    uint32_t queueFamilyQueueCount[maxQueueFamilies] = {};
    bool memoryBudgetExtension = false;
    bool calibratedTimestamps = false; // VK_KHR_calibrated_timestamps with the host clock (ADR 0051)
    bool presentation = false;         // swapchain + swapchain_maintenance1 + mutable format
    bool hdrMetadata = false;
    bool deviceFault = false; // VK_EXT_device_fault, enabled with crash diagnostics (ADR 0014)

    // Valid while initialized.
    VkDevice device = VK_NULL_HANDLE;
    DeviceFunctions fn;
    DescriptorHeap resourceHeap;
    DescriptorHeap samplerHeap;
    VkPipelineCache pipelineCache = VK_NULL_HANDLE; // internally synchronized (ADR 0043)
    Vector<std::mutex*> queueLocks;                 // one per VkQueue shared by several Slag queues (ADR 0009)
    // Distinct queue families in use; resources are CONCURRENT across them (ADR 0009).
    uint32_t sharingFamilies[3] = {};
    uint32_t sharingFamilyCount = 0;
    // Memory type for memory blocks of each MemoryKind: valid for every
    // resource type that kind may hold (ADR 0023).
    uint32_t blockMemoryTypes[4] = {};
};

struct CommandQueueImpl : CommandQueueCommon
{
    VkQueue queue = VK_NULL_HANDLE;
    uint32_t family = 0;
    std::mutex* sharedLock = nullptr; // set when the VkQueue is shared
    // Recorded once and put first in every submit: D3D12 finishes and flushes
    // all work at ExecuteCommandLists boundaries, and Slag's barriers rely on
    // it (ADR 0049).
    VkCommandPool boundaryPool = VK_NULL_HANDLE;
    VkCommandBuffer boundary = VK_NULL_HANDLE;
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
    // Reused by barrier and draw state.
    Vector<VkBufferMemoryBarrier2> bufferBarriers;
    Vector<VkImageMemoryBarrier2> imageBarriers;
    Vector<VkMemoryBarrier2> memoryBarriers;
};

struct MemoryBlockImpl : MemoryBlockCommon
{
    VkDeviceMemory memory = VK_NULL_HANDLE;
    uint8_t* mapped = nullptr; // CPU-visible kinds: mapped once, for the block's lifetime (ADR 0024)
};

struct BufferImpl : BufferCommon
{
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE; // own memory; null when placed
};

struct TextureImpl : TextureCommon
{
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE; // own memory; null when placed
    VkImageAspectFlags aspects = VK_IMAGE_ASPECT_COLOR_BIT;
    bool swapchainImage = false; // owned by its swapchain
};

// A binary semaphore and the fence that says when presentation is done with it
// (swapchain_maintenance1 present fences, ADR 0057).
struct PresentSlot
{
    VkSemaphore semaphore = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    bool used = false;
    VkSemaphore acquireSemaphore = VK_NULL_HANDLE; // the presented image's, free once the fence signals
};

struct SwapchainImpl : SwapchainCommon
{
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    // Acquire semaphores: a free pool, and the one each image was acquired
    // with until it is presented. A presented image's semaphore moves to its
    // present slot and is reused once the slot's present fence signals: the
    // present waited for all earlier work on its queue, which the work that
    // used the image (and waited on the semaphore) is ordered before. An image
    // acquired again doesn't prove that by itself (mailbox and immediate modes
    // hand images back early).
    Vector<VkSemaphore> freeAcquireSemaphores;
    Vector<VkSemaphore> imageAcquireSemaphores; // per image; may be null
    Vector<PresentSlot> presentSlots;
    uint32_t nextPresentSlot = 0;
    // Frame latency (ADR 0057).
    uint64_t presentId = 0;
    uint32_t maxFrameLatency = 2;
};

struct AttachmentViewImpl : AttachmentViewCommon
{
    VkImageView view = VK_NULL_HANDLE;
};

struct QueryPoolImpl : QueryPoolCommon
{
    VkQueryPool pool = VK_NULL_HANDLE;
};

struct SemaphoreImpl : SemaphoreCommon
{
    VkSemaphore semaphore = VK_NULL_HANDLE;
};

inline InstanceImpl& instanceOf(const GraphicsDeviceImpl& device)
{
    return *static_cast<InstanceImpl*>(device.instance);
}

VkCompareOp toCompareOp(CompareOp op);

// The queue family each CommandQueueType uses, indexed by type (ADR 0009).
void selectFamilies(const GraphicsDeviceImpl& device, uint32_t families[3]);

#ifdef _WIN32
constexpr VkTimeDomainKHR hostTimeDomain = VK_TIME_DOMAIN_QUERY_PERFORMANCE_COUNTER_KHR;
#else
constexpr VkTimeDomainKHR hostTimeDomain = VK_TIME_DOMAIN_CLOCK_MONOTONIC_KHR;
#endif

// The semaphore the swapchain's acquired image was acquired with; submits that
// list the swapchain wait on it (ADR 0057).
VkSemaphore acquireSemaphore(SwapchainCommon* swapchain);

// VkResult to Status; marks the device lost on VK_ERROR_DEVICE_LOST.
Status toStatus(GraphicsDeviceImpl& device, VkResult result);
// ADR 0049's layout mapping, for barriers Slag records itself.
VkImageLayout imageLayout(TextureLayout layout);

void setObjectName(GraphicsDeviceImpl& device, VkObjectType type, uint64_t handle, const char* name);

} // namespace slag::vk
