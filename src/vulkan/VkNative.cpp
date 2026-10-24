#include "vulkan/VkBackend.h"

#include <slag/native/Vulkan.h>

// Native interop accessors (ADR 0071).

namespace slag::vk {

namespace {

// Validation reports an object from another backend; the accessor then
// returns empty handles.
bool ours([[maybe_unused]] const InstanceCommon* instance, const BackendFunctions* functions,
          [[maybe_unused]] const char* type)
{
    bool vulkan = functions == slagBackend_vulkan()->functions;
    SLAG_VALIDATE(instance, vulkan, "slag::vk::native", "the %s belongs to another backend", type);
    return vulkan;
}

template <class Impl, class Public>
const Impl* impl(Public* object, const char* type)
{
    auto* common = static_cast<const Impl*>(object);
    return common && ours(common->device->instance, common->functions, type) ? common : nullptr;
}

// A placed resource's memory is its block's.
VkDeviceMemory memoryOf(VkDeviceMemory own, const MemoryBlockCommon* block)
{
    return own ? own : block ? static_cast<const MemoryBlockImpl*>(block)->memory : VK_NULL_HANDLE;
}

} // namespace

PFN_vkGetInstanceProcAddr getInstanceProcAddr(Instance* instance)
{
    auto* object = static_cast<InstanceImpl*>(instance);
    return object && ours(object, object->functions, "instance") ? object->global->vkGetInstanceProcAddr : nullptr;
}

VkInstance native(Instance* instance)
{
    auto* object = static_cast<InstanceImpl*>(instance);
    return object && ours(object, object->functions, "instance") ? object->instance : VK_NULL_HANDLE;
}

NativeDevice native(GraphicsDevice* device)
{
    auto* object = static_cast<GraphicsDeviceImpl*>(device);
    if (!object || !ours(object->instance, object->functions, "device"))
    {
        return {};
    }
    return {instanceOf(*object).instance, object->physicalDevice, object->device};
}

NativeQueue native(CommandQueue* queue)
{
    const CommandQueueImpl* object = impl<CommandQueueImpl>(queue, "queue");
    return object ? NativeQueue{object->queue, object->family} : NativeQueue{};
}

VkCommandBuffer native(CommandBuffer* commandBuffer)
{
    const CommandBufferImpl* object = impl<CommandBufferImpl>(commandBuffer, "command buffer");
    return object ? object->commandBuffer : VK_NULL_HANDLE;
}

VkDeviceMemory native(MemoryBlock* memoryBlock)
{
    const MemoryBlockImpl* object = impl<MemoryBlockImpl>(memoryBlock, "memory block");
    return object ? object->memory : VK_NULL_HANDLE;
}

NativeBuffer native(Buffer* buffer)
{
    const BufferImpl* object = impl<BufferImpl>(buffer, "buffer");
    if (!object)
    {
        return {};
    }
    return {object->buffer, memoryOf(object->memory, object->placementBlock), object->placementOffset};
}

NativeTexture native(Texture* texture)
{
    const TextureImpl* object = impl<TextureImpl>(texture, "texture");
    if (!object)
    {
        return {};
    }
    return {object->image, memoryOf(object->memory, object->placementBlock), object->placementOffset};
}

VkImageView native(AttachmentView* view)
{
    const AttachmentViewImpl* object = impl<AttachmentViewImpl>(view, "attachment view");
    return object ? object->view : VK_NULL_HANDLE;
}

VkPipeline native(Pipeline* pipeline)
{
    const PipelineImpl* object = impl<PipelineImpl>(pipeline, "pipeline");
    return object ? object->pipeline : VK_NULL_HANDLE;
}

VkQueryPool native(QueryPool* pool)
{
    const QueryPoolImpl* object = impl<QueryPoolImpl>(pool, "query pool");
    return object ? object->pool : VK_NULL_HANDLE;
}

VkSemaphore native(Semaphore* semaphore)
{
    const SemaphoreImpl* object = impl<SemaphoreImpl>(semaphore, "semaphore");
    return object ? object->semaphore : VK_NULL_HANDLE;
}

NativeSwapchain native(Swapchain* swapchain)
{
    const SwapchainImpl* object = impl<SwapchainImpl>(swapchain, "swapchain");
    return object ? NativeSwapchain{object->swapchain, object->surface} : NativeSwapchain{};
}

} // namespace slag::vk
