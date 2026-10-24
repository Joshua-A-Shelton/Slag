#pragma once

// Opt-in Vulkan interop (ADR 0071): the native objects behind Slag's.
// Includes vulkan/vulkan.h (Vulkan-Headers), so no other public header may
// include this one.
//
// Slag loads Vulkan itself (no vulkan-1 link): load your own entry points
// through getInstanceProcAddr(), with the native instance and device.
//
// The native objects stay Slag's: don't destroy them. A buffer or texture
// gets new ones when a defragmentation pass moves it (ADR 0062), swapchain
// images when the swapchain is reconfigured (ADR 0058).
//
// Native commands may be recorded into native(commandBuffer) between Slag
// calls. Slag then assumes only that its descriptor heaps (bound at begin())
// are still bound; set everything else again through Slag.

#include <slag/Slag.h>

#include <vulkan/vulkan.h>

namespace slag::vk {

struct NativeDevice
{
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE; // null until initialized
};

struct NativeQueue
{
    VkQueue queue = VK_NULL_HANDLE;
    uint32_t family = 0;
};

// Placed resources share their memory block's VkDeviceMemory.
struct NativeBuffer
{
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDeviceSize offset = 0;
};

struct NativeTexture
{
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE; // null for swapchain images
    VkDeviceSize offset = 0;
};

struct NativeSwapchain
{
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
};

PFN_vkGetInstanceProcAddr getInstanceProcAddr(Instance* instance);
VkInstance native(Instance* instance);
NativeDevice native(GraphicsDevice* device);
NativeQueue native(CommandQueue* queue);
VkCommandBuffer native(CommandBuffer* commandBuffer);
VkDeviceMemory native(MemoryBlock* memoryBlock);
NativeBuffer native(Buffer* buffer);
NativeTexture native(Texture* texture);
VkImageView native(AttachmentView* view);
VkPipeline native(Pipeline* pipeline);
VkQueryPool native(QueryPool* pool);
VkSemaphore native(Semaphore* semaphore);
NativeSwapchain native(Swapchain* swapchain);

} // namespace slag::vk
