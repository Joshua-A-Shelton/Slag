#include "common/Logging.h"
#include "vulkan/VkBackend.h"

namespace slag::vk {

namespace {

VkPipelineStageFlags2 toStages(BarrierSync sync)
{
    auto has = [sync](BarrierSync flag) {
        return (sync & flag) != BarrierSync::None;
    };
    VkPipelineStageFlags2 stages = VK_PIPELINE_STAGE_2_NONE;
    if (has(BarrierSync::All))
    {
        stages |= VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    }
    if (has(BarrierSync::Draw))
    {
        stages |= VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
    }
    if (has(BarrierSync::IndexInput))
    {
        stages |= VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT | VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT;
    }
    if (has(BarrierSync::VertexShading))
    {
        stages |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
    }
    if (has(BarrierSync::PixelShading))
    {
        stages |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    }
    if (has(BarrierSync::DepthStencil))
    {
        stages |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
    }
    if (has(BarrierSync::RenderTarget))
    {
        stages |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
    if (has(BarrierSync::ComputeShading))
    {
        stages |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    }
    if (has(BarrierSync::AllShading))
    {
        stages |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
                  VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    }
    if (has(BarrierSync::NonPixelShading))
    {
        stages |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    }
    if (has(BarrierSync::Copy))
    {
        stages |= VK_PIPELINE_STAGE_2_COPY_BIT;
    }
    if (has(BarrierSync::Resolve))
    {
        // In-pass resolves happen at color attachment output (ADR 0049).
        stages |= VK_PIPELINE_STAGE_2_RESOLVE_BIT | VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
    if (has(BarrierSync::ExecuteIndirect))
    {
        stages |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
    }
    return stages;
}

VkAccessFlags2 toAccess(BarrierAccess access)
{
    auto has = [access](BarrierAccess flag) {
        return (access & flag) != BarrierAccess::NoAccess;
    };
    VkAccessFlags2 flags = VK_ACCESS_2_NONE;
    if (has(BarrierAccess::VertexBuffer))
    {
        flags |= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
    }
    if (has(BarrierAccess::IndexBuffer))
    {
        flags |= VK_ACCESS_2_INDEX_READ_BIT;
    }
    if (has(BarrierAccess::ConstantBuffer))
    {
        flags |= VK_ACCESS_2_UNIFORM_READ_BIT;
    }
    if (has(BarrierAccess::ShaderRead))
    {
        flags |=
            VK_ACCESS_2_SHADER_SAMPLED_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_UNIFORM_READ_BIT;
    }
    if (has(BarrierAccess::ShaderWrite))
    {
        flags |= VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
    }
    if (has(BarrierAccess::RenderTarget))
    {
        flags |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    }
    if (has(BarrierAccess::DepthStencilRead))
    {
        flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    }
    if (has(BarrierAccess::DepthStencilWrite))
    {
        flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    if (has(BarrierAccess::IndirectArgument))
    {
        flags |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
    }
    if (has(BarrierAccess::CopySource))
    {
        flags |= VK_ACCESS_2_TRANSFER_READ_BIT;
    }
    if (has(BarrierAccess::CopyDest))
    {
        flags |= VK_ACCESS_2_TRANSFER_WRITE_BIT;
    }
    if (has(BarrierAccess::ResolveSource))
    {
        flags |= VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
    }
    if (has(BarrierAccess::ResolveDest))
    {
        flags |= VK_ACCESS_2_TRANSFER_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    }
    return flags;
}

GraphicsDeviceImpl& deviceOf(CommandBufferCommon* commandBuffer)
{
    return *static_cast<GraphicsDeviceImpl*>(commandBuffer->device);
}

VkBindHeapInfoEXT bindInfo(const DescriptorHeap& heap)
{
    VkBindHeapInfoEXT info = {VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT};
    info.heapRange.address = heap.address;
    info.heapRange.size = heap.reservedOffset + heap.reservedSize;
    info.reservedRangeOffset = heap.reservedOffset;
    info.reservedRangeSize = heap.reservedSize;
    return info;
}

} // namespace

Result<CommandBufferCommon*> createCommandBuffer(GraphicsDeviceCommon* common, const CommandBufferDesc& desc)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    auto* queue = static_cast<CommandQueueImpl*>(device.queues[static_cast<size_t>(desc.queueType)][0]);
    auto* commandBuffer = device.instance->allocator.create<CommandBufferImpl>();
    if (!commandBuffer)
    {
        return Status::OutOfHostMemory;
    }
    commandBuffer->device = &device;
    commandBuffer->functions = device.functions;
    commandBuffer->bindsHeaps =
        (device.queueFamilyFlags[queue->family] & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) != 0;
    commandBuffer->bufferBarriers = Vector<VkBufferMemoryBarrier2>(&device.instance->allocator);
    commandBuffer->memoryBarriers = Vector<VkMemoryBarrier2>(&device.instance->allocator);

    // Each command buffer owns its pool (ADR 0045).
    VkCommandPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.queueFamilyIndex = queue->family;
    VkResult result = device.fn.vkCreateCommandPool(device.device, &poolInfo, nullptr, &commandBuffer->pool);
    if (result == VK_SUCCESS)
    {
        VkCommandBufferAllocateInfo allocateInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        allocateInfo.commandPool = commandBuffer->pool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = 1;
        result = device.fn.vkAllocateCommandBuffers(device.device, &allocateInfo, &commandBuffer->commandBuffer);
    }
    if (result != VK_SUCCESS)
    {
        destroyCommandBuffer(&device, commandBuffer);
        return toStatus(device, result);
    }
    setObjectName(device, VK_OBJECT_TYPE_COMMAND_BUFFER, reinterpret_cast<uint64_t>(commandBuffer->commandBuffer),
                  desc.debugName);
    return static_cast<CommandBufferCommon*>(commandBuffer);
}

void destroyCommandBuffer(GraphicsDeviceCommon* common, CommandBufferCommon* object)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    auto* commandBuffer = static_cast<CommandBufferImpl*>(object);
    if (commandBuffer->pool != VK_NULL_HANDLE)
    {
        device.fn.vkDestroyCommandPool(device.device, commandBuffer->pool, nullptr); // frees the command buffer
    }
    device.instance->allocator.destroy(commandBuffer);
}

Result<void> beginCommandBuffer(CommandBufferCommon* object)
{
    auto* commandBuffer = static_cast<CommandBufferImpl*>(object);
    GraphicsDeviceImpl& device = deviceOf(object);
    VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VkResult result = device.fn.vkBeginCommandBuffer(commandBuffer->commandBuffer, &beginInfo);
    if (result != VK_SUCCESS)
    {
        return toStatus(device, result);
    }
    // Both heaps are bound at the start of every command buffer (ADR 0029).
    if (commandBuffer->bindsHeaps)
    {
        VkBindHeapInfoEXT resourceHeap = bindInfo(device.resourceHeap);
        VkBindHeapInfoEXT samplerHeap = bindInfo(device.samplerHeap);
        device.fn.vkCmdBindResourceHeapEXT(commandBuffer->commandBuffer, &resourceHeap);
        device.fn.vkCmdBindSamplerHeapEXT(commandBuffer->commandBuffer, &samplerHeap);
    }
    return {};
}

Result<void> endCommandBuffer(CommandBufferCommon* object)
{
    auto* commandBuffer = static_cast<CommandBufferImpl*>(object);
    GraphicsDeviceImpl& device = deviceOf(object);

    // Makes GPU writes visible to the CPU after a semaphore wait, as on D3D12
    // (ADR 0049).
    VkMemoryBarrier2 hostVisibility = {VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
    hostVisibility.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    hostVisibility.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
    hostVisibility.dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT;
    hostVisibility.dstAccessMask = VK_ACCESS_2_HOST_READ_BIT;
    VkDependencyInfo dependency = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency.memoryBarrierCount = 1;
    dependency.pMemoryBarriers = &hostVisibility;
    device.fn.vkCmdPipelineBarrier2(commandBuffer->commandBuffer, &dependency);

    return toStatus(device, device.fn.vkEndCommandBuffer(commandBuffer->commandBuffer));
}

Result<void> resetCommandBuffer(CommandBufferCommon* object)
{
    auto* commandBuffer = static_cast<CommandBufferImpl*>(object);
    GraphicsDeviceImpl& device = deviceOf(object);
    return toStatus(device, device.fn.vkResetCommandPool(device.device, commandBuffer->pool, 0));
}

void cmdBindPipeline(CommandBufferCommon* object, PipelineCommon* pipelineObject)
{
    auto* commandBuffer = static_cast<CommandBufferImpl*>(object);
    auto* pipeline = static_cast<PipelineImpl*>(pipelineObject);
    deviceOf(object).fn.vkCmdBindPipeline(commandBuffer->commandBuffer, pipeline->bindPoint, pipeline->pipeline);
}

void cmdPushConstants(CommandBufferCommon* object, const void* data, uint32_t size, uint32_t offset)
{
    auto* commandBuffer = static_cast<CommandBufferImpl*>(object);
    // Push constants are push data with descriptor heaps (ADR 0003).
    VkPushDataInfoEXT info = {VK_STRUCTURE_TYPE_PUSH_DATA_INFO_EXT};
    info.offset = offset;
    info.data.address = data;
    info.data.size = size;
    deviceOf(object).fn.vkCmdPushDataEXT(commandBuffer->commandBuffer, &info);
}

void cmdDispatch(CommandBufferCommon* object, uint32_t x, uint32_t y, uint32_t z)
{
    auto* commandBuffer = static_cast<CommandBufferImpl*>(object);
    deviceOf(object).fn.vkCmdDispatch(commandBuffer->commandBuffer, x, y, z);
}

void cmdBarrier(CommandBufferCommon* object, std::span<const GlobalBarrier> globalBarriers,
                std::span<const BufferBarrier> bufferBarriers, std::span<const TextureBarrier>)
{
    auto* commandBuffer = static_cast<CommandBufferImpl*>(object);
    GraphicsDeviceImpl& device = deviceOf(object);
    commandBuffer->memoryBarriers.clear();
    commandBuffer->bufferBarriers.clear();
    for (const GlobalBarrier& barrier : globalBarriers)
    {
        VkMemoryBarrier2 vkBarrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
        vkBarrier.srcStageMask = toStages(barrier.syncBefore);
        vkBarrier.srcAccessMask = toAccess(barrier.accessBefore);
        vkBarrier.dstStageMask = toStages(barrier.syncAfter);
        vkBarrier.dstAccessMask = toAccess(barrier.accessAfter);
        if (!commandBuffer->memoryBarriers.pushBack(vkBarrier))
        {
            return; // out of host memory; reported by the allocator's failure
        }
    }
    for (const BufferBarrier& barrier : bufferBarriers)
    {
        VkBufferMemoryBarrier2 vkBarrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
        vkBarrier.srcStageMask = toStages(barrier.syncBefore);
        vkBarrier.srcAccessMask = toAccess(barrier.accessBefore);
        vkBarrier.dstStageMask = toStages(barrier.syncAfter);
        vkBarrier.dstAccessMask = toAccess(barrier.accessAfter);
        vkBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vkBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vkBarrier.buffer = static_cast<BufferImpl*>(barrier.buffer)->buffer;
        vkBarrier.offset = 0;
        vkBarrier.size = VK_WHOLE_SIZE;
        if (!commandBuffer->bufferBarriers.pushBack(vkBarrier))
        {
            return;
        }
    }
    VkDependencyInfo dependency = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency.memoryBarrierCount = static_cast<uint32_t>(commandBuffer->memoryBarriers.size());
    dependency.pMemoryBarriers = commandBuffer->memoryBarriers.data();
    dependency.bufferMemoryBarrierCount = static_cast<uint32_t>(commandBuffer->bufferBarriers.size());
    dependency.pBufferMemoryBarriers = commandBuffer->bufferBarriers.data();
    device.fn.vkCmdPipelineBarrier2(commandBuffer->commandBuffer, &dependency);
}

void cmdCopyBuffer(CommandBufferCommon* object, BufferCommon* destination, uint64_t destinationOffset,
                   BufferCommon* source, uint64_t sourceOffset, uint64_t size)
{
    auto* commandBuffer = static_cast<CommandBufferImpl*>(object);
    VkBufferCopy region = {sourceOffset, destinationOffset, size};
    deviceOf(object).fn.vkCmdCopyBuffer(commandBuffer->commandBuffer, static_cast<BufferImpl*>(source)->buffer,
                                        static_cast<BufferImpl*>(destination)->buffer, 1, &region);
}

Result<void> submit(CommandQueueCommon* object, const SubmitDesc& desc)
{
    auto* queue = static_cast<CommandQueueImpl*>(object);
    auto& device = *static_cast<GraphicsDeviceImpl*>(queue->device);
    queue->waitInfos.clear();
    queue->signalInfos.clear();
    queue->commandBufferInfos.clear();

    // Waits hold back all of the submit's work, as D3D12's queue Wait does (ADR 0053).
    for (const SemaphoreValue& wait : desc.waits)
    {
        VkSemaphoreSubmitInfo info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
        info.semaphore = static_cast<SemaphoreImpl*>(wait.semaphore)->semaphore;
        info.value = wait.value;
        info.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        if (!queue->waitInfos.pushBack(info))
        {
            return Status::OutOfHostMemory;
        }
    }
    for (const SemaphoreValue& signal : desc.signals)
    {
        VkSemaphoreSubmitInfo info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
        info.semaphore = static_cast<SemaphoreImpl*>(signal.semaphore)->semaphore;
        info.value = signal.value;
        info.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        if (!queue->signalInfos.pushBack(info))
        {
            return Status::OutOfHostMemory;
        }
    }
    for (CommandBuffer* commandBuffer : desc.commandBuffers)
    {
        VkCommandBufferSubmitInfo info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
        info.commandBuffer = static_cast<CommandBufferImpl*>(commandBuffer)->commandBuffer;
        if (!queue->commandBufferInfos.pushBack(info))
        {
            return Status::OutOfHostMemory;
        }
    }

    VkSubmitInfo2 submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    submitInfo.waitSemaphoreInfoCount = static_cast<uint32_t>(queue->waitInfos.size());
    submitInfo.pWaitSemaphoreInfos = queue->waitInfos.data();
    submitInfo.commandBufferInfoCount = static_cast<uint32_t>(queue->commandBufferInfos.size());
    submitInfo.pCommandBufferInfos = queue->commandBufferInfos.data();
    submitInfo.signalSemaphoreInfoCount = static_cast<uint32_t>(queue->signalInfos.size());
    submitInfo.pSignalSemaphoreInfos = queue->signalInfos.data();

    VkResult result;
    if (queue->sharedLock)
    {
        std::lock_guard<std::mutex> lock(*queue->sharedLock);
        result = device.fn.vkQueueSubmit2(queue->queue, 1, &submitInfo, VK_NULL_HANDLE);
    }
    else
    {
        result = device.fn.vkQueueSubmit2(queue->queue, 1, &submitInfo, VK_NULL_HANDLE);
    }
    return toStatus(device, result);
}

} // namespace slag::vk
