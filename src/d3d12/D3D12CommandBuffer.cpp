#include "d3d12/D3D12Backend.h"

namespace slag::d3d12 {

namespace {

// Slag's barrier model is Enhanced Barriers (ADR 0049); the bits map one to one.
D3D12_BARRIER_SYNC toSync(BarrierSync sync)
{
    struct Entry
    {
        BarrierSync slag;
        D3D12_BARRIER_SYNC d3d12;
    };
    static constexpr Entry entries[] = {
        {BarrierSync::All, D3D12_BARRIER_SYNC_ALL},
        {BarrierSync::Draw, D3D12_BARRIER_SYNC_DRAW},
        {BarrierSync::IndexInput, D3D12_BARRIER_SYNC_INDEX_INPUT},
        {BarrierSync::VertexShading, D3D12_BARRIER_SYNC_VERTEX_SHADING},
        {BarrierSync::PixelShading, D3D12_BARRIER_SYNC_PIXEL_SHADING},
        {BarrierSync::DepthStencil, D3D12_BARRIER_SYNC_DEPTH_STENCIL},
        {BarrierSync::RenderTarget, D3D12_BARRIER_SYNC_RENDER_TARGET},
        {BarrierSync::ComputeShading, D3D12_BARRIER_SYNC_COMPUTE_SHADING},
        {BarrierSync::AllShading, D3D12_BARRIER_SYNC_ALL_SHADING},
        {BarrierSync::NonPixelShading, D3D12_BARRIER_SYNC_NON_PIXEL_SHADING},
        {BarrierSync::Copy, D3D12_BARRIER_SYNC_COPY},
        {BarrierSync::Resolve, D3D12_BARRIER_SYNC_RESOLVE},
        {BarrierSync::ExecuteIndirect, D3D12_BARRIER_SYNC_EXECUTE_INDIRECT},
    };
    uint32_t bits = 0;
    for (const Entry& entry : entries)
    {
        if ((sync & entry.slag) != BarrierSync::None)
        {
            bits |= static_cast<uint32_t>(entry.d3d12);
        }
    }
    return static_cast<D3D12_BARRIER_SYNC>(bits);
}

D3D12_BARRIER_ACCESS toAccess(BarrierAccess access)
{
    // Slag's NoAccess is D3D12's NO_ACCESS bit, not ACCESS_COMMON (which is 0).
    if (access == BarrierAccess::NoAccess)
    {
        return D3D12_BARRIER_ACCESS_NO_ACCESS;
    }
    struct Entry
    {
        BarrierAccess slag;
        D3D12_BARRIER_ACCESS d3d12;
    };
    static constexpr Entry entries[] = {
        {BarrierAccess::VertexBuffer, D3D12_BARRIER_ACCESS_VERTEX_BUFFER},
        {BarrierAccess::IndexBuffer, D3D12_BARRIER_ACCESS_INDEX_BUFFER},
        {BarrierAccess::ConstantBuffer, D3D12_BARRIER_ACCESS_CONSTANT_BUFFER},
        {BarrierAccess::ShaderRead, D3D12_BARRIER_ACCESS_SHADER_RESOURCE},
        {BarrierAccess::ShaderWrite, D3D12_BARRIER_ACCESS_UNORDERED_ACCESS},
        {BarrierAccess::RenderTarget, D3D12_BARRIER_ACCESS_RENDER_TARGET},
        {BarrierAccess::DepthStencilRead, D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ},
        {BarrierAccess::DepthStencilWrite, D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE},
        {BarrierAccess::IndirectArgument, D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT},
        {BarrierAccess::CopySource, D3D12_BARRIER_ACCESS_COPY_SOURCE},
        {BarrierAccess::CopyDest, D3D12_BARRIER_ACCESS_COPY_DEST},
        {BarrierAccess::ResolveSource, D3D12_BARRIER_ACCESS_RESOLVE_SOURCE},
        {BarrierAccess::ResolveDest, D3D12_BARRIER_ACCESS_RESOLVE_DEST},
    };
    uint32_t bits = 0;
    for (const Entry& entry : entries)
    {
        if ((access & entry.slag) != BarrierAccess::NoAccess)
        {
            bits |= static_cast<uint32_t>(entry.d3d12);
        }
    }
    return static_cast<D3D12_BARRIER_ACCESS>(bits);
}

GraphicsDeviceImpl& deviceOf(CommandBufferCommon* commandBuffer)
{
    return *static_cast<GraphicsDeviceImpl*>(commandBuffer->device);
}

} // namespace

Result<CommandBufferCommon*> createCommandBuffer(GraphicsDeviceCommon* common, const CommandBufferDesc& desc)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    auto* commandBuffer = device.instance->allocator.create<CommandBufferImpl>();
    if (!commandBuffer)
    {
        return Status::OutOfHostMemory;
    }
    commandBuffer->device = &device;
    commandBuffer->functions = device.functions;
    commandBuffer->type = listType(desc.queueType);
    commandBuffer->globalBarriers = Vector<D3D12_GLOBAL_BARRIER>(&device.instance->allocator);
    commandBuffer->bufferBarriers = Vector<D3D12_BUFFER_BARRIER>(&device.instance->allocator);

    // Each command buffer owns its allocator (ADR 0045). CreateCommandList1
    // creates the list closed, so begin() can always Reset it.
    HRESULT result =
        device.device->CreateCommandAllocator(commandBuffer->type, IID_PPV_ARGS(&commandBuffer->allocator));
    if (SUCCEEDED(result))
    {
        result = device.device->CreateCommandList1(0, commandBuffer->type, D3D12_COMMAND_LIST_FLAG_NONE,
                                                   IID_PPV_ARGS(&commandBuffer->list));
    }
    if (FAILED(result))
    {
        destroyCommandBuffer(&device, commandBuffer);
        return toStatus(device, result);
    }
    setObjectName(commandBuffer->list, desc.debugName);
    return static_cast<CommandBufferCommon*>(commandBuffer);
}

void destroyCommandBuffer(GraphicsDeviceCommon* common, CommandBufferCommon* object)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    auto* commandBuffer = static_cast<CommandBufferImpl*>(object);
    release(commandBuffer->list);
    release(commandBuffer->allocator);
    device.instance->allocator.destroy(commandBuffer);
}

Result<void> beginCommandBuffer(CommandBufferCommon* object)
{
    auto* commandBuffer = static_cast<CommandBufferImpl*>(object);
    GraphicsDeviceImpl& device = deviceOf(object);
    HRESULT result = commandBuffer->list->Reset(commandBuffer->allocator, nullptr);
    if (FAILED(result))
    {
        return toStatus(device, result);
    }
    // Both heaps and the fixed root signature are bound at the start of every
    // command list (ADRs 0003, 0029). Copy lists have neither.
    if (commandBuffer->type != D3D12_COMMAND_LIST_TYPE_COPY)
    {
        ID3D12DescriptorHeap* heaps[] = {device.resourceHeap.heap, device.samplerHeap.heap};
        commandBuffer->list->SetDescriptorHeaps(2, heaps);
        commandBuffer->list->SetComputeRootSignature(device.rootSignature);
        if (commandBuffer->type == D3D12_COMMAND_LIST_TYPE_DIRECT)
        {
            commandBuffer->list->SetGraphicsRootSignature(device.rootSignature);
        }
    }
    return {};
}

Result<void> endCommandBuffer(CommandBufferCommon* object)
{
    auto* commandBuffer = static_cast<CommandBufferImpl*>(object);
    return toStatus(deviceOf(object), commandBuffer->list->Close());
}

Result<void> resetCommandBuffer(CommandBufferCommon* object)
{
    auto* commandBuffer = static_cast<CommandBufferImpl*>(object);
    return toStatus(deviceOf(object), commandBuffer->allocator->Reset());
}

void cmdBindPipeline(CommandBufferCommon* object, PipelineCommon* pipelineObject)
{
    auto* commandBuffer = static_cast<CommandBufferImpl*>(object);
    commandBuffer->list->SetPipelineState(static_cast<PipelineImpl*>(pipelineObject)->pipeline);
}

void cmdPushConstants(CommandBufferCommon* object, const void* data, uint32_t size, uint32_t offset)
{
    auto* commandBuffer = static_cast<CommandBufferImpl*>(object);
    // Vulkan push data reaches every pipeline type; D3D12 keeps graphics and
    // compute root arguments apart, so direct lists set both (ADR 0003).
    commandBuffer->list->SetComputeRoot32BitConstants(0, size / 4, data, offset / 4);
    if (commandBuffer->type == D3D12_COMMAND_LIST_TYPE_DIRECT)
    {
        commandBuffer->list->SetGraphicsRoot32BitConstants(0, size / 4, data, offset / 4);
    }
}

void cmdDispatch(CommandBufferCommon* object, uint32_t x, uint32_t y, uint32_t z)
{
    static_cast<CommandBufferImpl*>(object)->list->Dispatch(x, y, z);
}

void cmdBarrier(CommandBufferCommon* object, std::span<const GlobalBarrier> globalBarriers,
                std::span<const BufferBarrier> bufferBarriers, std::span<const TextureBarrier>)
{
    auto* commandBuffer = static_cast<CommandBufferImpl*>(object);
    commandBuffer->globalBarriers.clear();
    commandBuffer->bufferBarriers.clear();
    for (const GlobalBarrier& barrier : globalBarriers)
    {
        D3D12_GLOBAL_BARRIER d3d12Barrier = {toSync(barrier.syncBefore), toSync(barrier.syncAfter),
                                             toAccess(barrier.accessBefore), toAccess(barrier.accessAfter)};
        if (!commandBuffer->globalBarriers.pushBack(d3d12Barrier))
        {
            return; // out of host memory; reported by the allocator's failure
        }
    }
    for (const BufferBarrier& barrier : bufferBarriers)
    {
        // Buffer barriers always cover the whole buffer (ADR 0049).
        D3D12_BUFFER_BARRIER d3d12Barrier = {toSync(barrier.syncBefore),
                                             toSync(barrier.syncAfter),
                                             toAccess(barrier.accessBefore),
                                             toAccess(barrier.accessAfter),
                                             static_cast<BufferImpl*>(barrier.buffer)->resource,
                                             0,
                                             UINT64_MAX};
        if (!commandBuffer->bufferBarriers.pushBack(d3d12Barrier))
        {
            return;
        }
    }
    D3D12_BARRIER_GROUP groups[2] = {};
    UINT32 groupCount = 0;
    if (!commandBuffer->globalBarriers.empty())
    {
        groups[groupCount].Type = D3D12_BARRIER_TYPE_GLOBAL;
        groups[groupCount].NumBarriers = static_cast<UINT32>(commandBuffer->globalBarriers.size());
        groups[groupCount].pGlobalBarriers = commandBuffer->globalBarriers.data();
        ++groupCount;
    }
    if (!commandBuffer->bufferBarriers.empty())
    {
        groups[groupCount].Type = D3D12_BARRIER_TYPE_BUFFER;
        groups[groupCount].NumBarriers = static_cast<UINT32>(commandBuffer->bufferBarriers.size());
        groups[groupCount].pBufferBarriers = commandBuffer->bufferBarriers.data();
        ++groupCount;
    }
    commandBuffer->list->Barrier(groupCount, groups);
}

void cmdCopyBuffer(CommandBufferCommon* object, BufferCommon* destination, uint64_t destinationOffset,
                   BufferCommon* source, uint64_t sourceOffset, uint64_t size)
{
    static_cast<CommandBufferImpl*>(object)->list->CopyBufferRegion(
        static_cast<BufferImpl*>(destination)->resource, destinationOffset, static_cast<BufferImpl*>(source)->resource,
        sourceOffset, size);
}

Result<void> submit(CommandQueueCommon* object, const SubmitDesc& desc)
{
    auto* queue = static_cast<CommandQueueImpl*>(object);
    auto& device = *static_cast<GraphicsDeviceImpl*>(queue->device);
    queue->commandLists.clear();
    for (CommandBuffer* commandBuffer : desc.commandBuffers)
    {
        if (!queue->commandLists.pushBack(static_cast<CommandBufferImpl*>(commandBuffer)->list))
        {
            return Status::OutOfHostMemory;
        }
    }

    // Waits hold back everything submitted after them (ADR 0053).
    for (const SemaphoreValue& wait : desc.waits)
    {
        HRESULT result = queue->queue->Wait(static_cast<SemaphoreImpl*>(wait.semaphore)->fence, wait.value);
        if (FAILED(result))
        {
            return toStatus(device, result);
        }
    }
    if (!queue->commandLists.empty())
    {
        queue->queue->ExecuteCommandLists(static_cast<UINT>(queue->commandLists.size()), queue->commandLists.data());
    }
    for (const SemaphoreValue& signal : desc.signals)
    {
        HRESULT result = queue->queue->Signal(static_cast<SemaphoreImpl*>(signal.semaphore)->fence, signal.value);
        if (FAILED(result))
        {
            return toStatus(device, result);
        }
    }
    return {};
}

} // namespace slag::d3d12
