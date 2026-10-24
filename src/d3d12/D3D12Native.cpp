#include "d3d12/D3D12Backend.h"

#include <slag/native/D3D12.h>

// Native interop accessors (ADR 0071).

namespace slag::d3d12 {

namespace {

// Validation reports an object from another backend; the accessor then
// returns empty handles.
bool ours([[maybe_unused]] const InstanceCommon* instance, const BackendFunctions* functions,
          [[maybe_unused]] const char* type)
{
    bool d3d12 = functions == slagBackend_d3d12()->functions;
    SLAG_VALIDATE(instance, d3d12, "slag::d3d12::native", "the %s belongs to another backend", type);
    return d3d12;
}

template <class Impl, class Public>
const Impl* impl(Public* object, const char* type)
{
    auto* common = static_cast<const Impl*>(object);
    return common && ours(common->device->instance, common->functions, type) ? common : nullptr;
}

} // namespace

IDXGIFactory6* native(Instance* instance)
{
    auto* object = static_cast<InstanceImpl*>(instance);
    return object && ours(object, object->functions, "instance") ? object->factory : nullptr;
}

NativeDevice native(GraphicsDevice* device)
{
    auto* object = static_cast<GraphicsDeviceImpl*>(device);
    if (!object || !ours(object->instance, object->functions, "device"))
    {
        return {};
    }
    return {object->device, object->adapter, object->resourceHeap.heap, object->samplerHeap.heap,
            object->rootSignature};
}

ID3D12CommandQueue* native(CommandQueue* queue)
{
    const CommandQueueImpl* object = impl<CommandQueueImpl>(queue, "queue");
    return object ? object->queue : nullptr;
}

ID3D12GraphicsCommandList7* native(CommandBuffer* commandBuffer)
{
    const CommandBufferImpl* object = impl<CommandBufferImpl>(commandBuffer, "command buffer");
    return object ? object->list : nullptr;
}

ID3D12Heap* native(MemoryBlock* memoryBlock)
{
    const MemoryBlockImpl* object = impl<MemoryBlockImpl>(memoryBlock, "memory block");
    return object ? object->heap : nullptr;
}

ID3D12Resource* native(Buffer* buffer)
{
    const BufferImpl* object = impl<BufferImpl>(buffer, "buffer");
    return object ? object->resource : nullptr;
}

ID3D12Resource* native(Texture* texture)
{
    const TextureImpl* object = impl<TextureImpl>(texture, "texture");
    return object ? object->resource : nullptr;
}

D3D12_CPU_DESCRIPTOR_HANDLE native(AttachmentView* view)
{
    const AttachmentViewImpl* object = impl<AttachmentViewImpl>(view, "attachment view");
    return object ? object->handle : D3D12_CPU_DESCRIPTOR_HANDLE{};
}

ID3D12PipelineState* native(Pipeline* pipeline)
{
    const PipelineImpl* object = impl<PipelineImpl>(pipeline, "pipeline");
    return object ? object->pipeline : nullptr;
}

ID3D12QueryHeap* native(QueryPool* pool)
{
    const QueryPoolImpl* object = impl<QueryPoolImpl>(pool, "query pool");
    return object ? object->heap : nullptr;
}

ID3D12Fence* native(Semaphore* semaphore)
{
    const SemaphoreImpl* object = impl<SemaphoreImpl>(semaphore, "semaphore");
    return object ? object->fence : nullptr;
}

IDXGISwapChain4* native(Swapchain* swapchain)
{
    const SwapchainImpl* object = impl<SwapchainImpl>(swapchain, "swapchain");
    return object ? object->swapchain : nullptr;
}

} // namespace slag::d3d12
