#include "common/Logging.h"
#include "common/Objects.h"

#include <slag/CommandBuffer.h>
#include <slag/GraphicsDevice.h>
#include <slag/Limits.h>

namespace slag {

namespace {

CommandBufferCommon* common(CommandBuffer* commandBuffer)
{
    return static_cast<CommandBufferCommon*>(commandBuffer);
}

[[maybe_unused]] const char* stateName(CommandBufferState state)
{
    switch (state)
    {
    case CommandBufferState::Initial:
        return "not begun";
    case CommandBufferState::Recording:
        return "recording";
    case CommandBufferState::Executable:
        return "ended";
    }
    return "?";
}

} // namespace

#define SLAG_VALIDATE_RECORDING(self, function)                                                                        \
    SLAG_VALIDATE((self)->device->instance, (self)->state == CommandBufferState::Recording, function,                  \
                  "the command buffer is %s, not recording", stateName((self)->state))

// --- Creation (GraphicsDevice) ----------------------------------------------

Result<CommandBuffer*> GraphicsDevice::createCommandBuffer(const CommandBufferDesc& desc)
{
    auto* self = static_cast<GraphicsDeviceCommon*>(this);
    SLAG_VALIDATE(self->instance, self->initialized, "GraphicsDevice::createCommandBuffer",
                  "device \"%s\" is not initialized", self->info.name);
    SLAG_VALIDATE(self->instance, !self->queues[static_cast<size_t>(desc.queueType)].empty(),
                  "GraphicsDevice::createCommandBuffer", "the device has no queue of this type");
    if (self->lost)
    {
        return Status::DeviceLost;
    }
    Result<CommandBufferCommon*> commandBuffer = SLAG_DISPATCH(self, createCommandBuffer)(self, desc);
    if (!commandBuffer)
    {
        return commandBuffer.status();
    }
    commandBuffer.value()->userData = desc.userData;
    commandBuffer.value()->queueType = desc.queueType;
    return static_cast<CommandBuffer*>(commandBuffer.value());
}

void GraphicsDevice::destroy(CommandBuffer* commandBuffer)
{
    if (!commandBuffer)
    {
        return;
    }
    auto* self = static_cast<GraphicsDeviceCommon*>(this);
    CommandBufferCommon* object = common(commandBuffer);
    SLAG_VALIDATE(self->instance, object->device == self, "GraphicsDevice::destroy",
                  "the command buffer belongs to another device");
    SLAG_DISPATCH(self, destroyCommandBuffer)(self, object);
}

// --- Recording --------------------------------------------------------------

Result<void> CommandBuffer::begin()
{
    CommandBufferCommon* self = common(this);
    SLAG_VALIDATE(self->device->instance, self->state == CommandBufferState::Initial, "CommandBuffer::begin",
                  "the command buffer is %s; reset it first", stateName(self->state));
    Result<void> result = SLAG_DISPATCH(self, beginCommandBuffer)(self);
    if (result)
    {
        self->state = CommandBufferState::Recording;
        self->boundPipeline = nullptr;
    }
    return result;
}

Result<void> CommandBuffer::end()
{
    CommandBufferCommon* self = common(this);
    SLAG_VALIDATE_RECORDING(self, "CommandBuffer::end");
    Result<void> result = SLAG_DISPATCH(self, endCommandBuffer)(self);
    if (result)
    {
        self->state = CommandBufferState::Executable;
    }
    return result;
}

Result<void> CommandBuffer::reset()
{
    CommandBufferCommon* self = common(this);
    Result<void> result = SLAG_DISPATCH(self, resetCommandBuffer)(self);
    if (result)
    {
        self->state = CommandBufferState::Initial;
        self->boundPipeline = nullptr;
    }
    return result;
}

void CommandBuffer::bindPipeline(Pipeline* pipeline)
{
    CommandBufferCommon* self = common(this);
    SLAG_VALIDATE_RECORDING(self, "CommandBuffer::bindPipeline");
    SLAG_VALIDATE(self->device->instance, pipeline, "CommandBuffer::bindPipeline", "pipeline is null");
    auto* object = static_cast<PipelineCommon*>(pipeline);
    self->boundPipeline = object;
    SLAG_DISPATCH(self, cmdBindPipeline)(self, object);
}

void CommandBuffer::pushConstants(const void* data, uint32_t size, uint32_t offset)
{
    CommandBufferCommon* self = common(this);
    SLAG_VALIDATE_RECORDING(self, "CommandBuffer::pushConstants");
    SLAG_VALIDATE(self->device->instance, offset + size <= kMaxPushConstantsSize, "CommandBuffer::pushConstants",
                  "offset %u + size %u exceeds kMaxPushConstantsSize (%u)", offset, size, kMaxPushConstantsSize);
    SLAG_VALIDATE(self->device->instance, offset % 4 == 0 && size % 4 == 0, "CommandBuffer::pushConstants",
                  "offset and size must be multiples of 4 (offset %u, size %u)", offset, size);
    SLAG_DISPATCH(self, cmdPushConstants)(self, data, size, offset);
}

void CommandBuffer::dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    CommandBufferCommon* self = common(this);
    SLAG_VALIDATE_RECORDING(self, "CommandBuffer::dispatch");
    SLAG_VALIDATE(self->device->instance, self->boundPipeline && self->boundPipeline->kind == PipelineKind::Compute,
                  "CommandBuffer::dispatch", "no compute pipeline is bound");
    SLAG_DISPATCH(self, cmdDispatch)(self, groupCountX, groupCountY, groupCountZ);
}

void CommandBuffer::barrier(std::span<const GlobalBarrier> globalBarriers,
                            std::span<const BufferBarrier> bufferBarriers,
                            [[maybe_unused]] std::span<const TextureBarrier> textureBarriers)
{
    CommandBufferCommon* self = common(this);
    SLAG_VALIDATE_RECORDING(self, "CommandBuffer::barrier");
    SLAG_VALIDATE(self->device->instance, textureBarriers.empty(), "CommandBuffer::barrier",
                  "texture barriers arrive with textures in milestone M2");
    if (globalBarriers.empty() && bufferBarriers.empty())
    {
        return;
    }
    SLAG_DISPATCH(self, cmdBarrier)(self, globalBarriers, bufferBarriers, {});
}

void CommandBuffer::copyBuffer(Buffer* destination, uint64_t destinationOffset, Buffer* source, uint64_t sourceOffset,
                               uint64_t size)
{
    CommandBufferCommon* self = common(this);
    auto* dst = static_cast<BufferCommon*>(destination);
    auto* src = static_cast<BufferCommon*>(source);
    SLAG_VALIDATE_RECORDING(self, "CommandBuffer::copyBuffer");
    SLAG_VALIDATE(self->device->instance, dst && src, "CommandBuffer::copyBuffer", "a buffer is null");
    SLAG_VALIDATE(self->device->instance, !dst || destinationOffset + size <= dst->size, "CommandBuffer::copyBuffer",
                  "the destination range ends past the buffer");
    SLAG_VALIDATE(self->device->instance, !src || sourceOffset + size <= src->size, "CommandBuffer::copyBuffer",
                  "the source range ends past the buffer");
    SLAG_DISPATCH(self, cmdCopyBuffer)(self, dst, destinationOffset, src, sourceOffset, size);
}

uint64_t CommandBuffer::userData() const
{
    return static_cast<const CommandBufferCommon*>(this)->userData;
}

void CommandBuffer::setUserData(uint64_t value)
{
    common(this)->userData = value;
}

// --- Submission (CommandQueue) ----------------------------------------------

Result<void> CommandQueue::submit(const SubmitDesc& desc)
{
    auto* self = static_cast<CommandQueueCommon*>(this);
#ifdef SLAG_VALIDATION
    for (CommandBuffer* commandBuffer : desc.commandBuffers)
    {
        CommandBufferCommon* object = common(commandBuffer);
        SLAG_VALIDATE(self->device->instance, object->state == CommandBufferState::Executable, "CommandQueue::submit",
                      "a command buffer is %s, not ended", stateName(object->state));
        SLAG_VALIDATE(self->device->instance, object->queueType == self->type, "CommandQueue::submit",
                      "a command buffer was created for a different queue type");
    }
#endif
    if (self->device->lost)
    {
        return Status::DeviceLost;
    }
    return SLAG_DISPATCH(self, submit)(self, desc);
}

// --- Pipelines (GraphicsDevice, Pipeline) -----------------------------------

Result<Pipeline*> GraphicsDevice::createComputePipeline(const ComputePipelineDesc& desc)
{
    auto* self = static_cast<GraphicsDeviceCommon*>(this);
    SLAG_VALIDATE(self->instance, self->initialized, "GraphicsDevice::createComputePipeline",
                  "device \"%s\" is not initialized", self->info.name);
    SLAG_VALIDATE(self->instance, !desc.compute.binaries.empty(), "GraphicsDevice::createComputePipeline",
                  "no shader binaries");
    if (self->lost)
    {
        return Status::DeviceLost;
    }
    Result<PipelineCommon*> pipeline = SLAG_DISPATCH(self, createComputePipeline)(self, desc);
    if (!pipeline)
    {
        return pipeline.status();
    }
    pipeline.value()->userData = desc.userData;
    pipeline.value()->kind = PipelineKind::Compute;
    return static_cast<Pipeline*>(pipeline.value());
}

void GraphicsDevice::destroy(Pipeline* pipeline)
{
    if (!pipeline)
    {
        return;
    }
    auto* self = static_cast<GraphicsDeviceCommon*>(this);
    auto* object = static_cast<PipelineCommon*>(pipeline);
    SLAG_VALIDATE(self->instance, object->device == self, "GraphicsDevice::destroy",
                  "the pipeline belongs to another device");
    SLAG_DISPATCH(self, destroyPipeline)(self, object);
}

uint64_t Pipeline::userData() const
{
    return static_cast<const PipelineCommon*>(this)->userData;
}

void Pipeline::setUserData(uint64_t value)
{
    static_cast<PipelineCommon*>(this)->userData = value;
}

// --- Descriptors (GraphicsDevice) -------------------------------------------

void GraphicsDevice::writeRWStructuredBuffer(uint32_t index, const StructuredBufferDescriptor& descriptor)
{
    auto* self = static_cast<GraphicsDeviceCommon*>(this);
    [[maybe_unused]] const auto* buffer = static_cast<const BufferCommon*>(descriptor.buffer);
    SLAG_VALIDATE(self->instance, index < self->resourceHeapSize, "GraphicsDevice::writeRWStructuredBuffer",
                  "slot %u is outside the resource heap (%u slots)", index, self->resourceHeapSize);
    SLAG_VALIDATE(self->instance, buffer, "GraphicsDevice::writeRWStructuredBuffer", "buffer is null");
    SLAG_VALIDATE(self->instance, descriptor.stride > 0 && descriptor.stride % 4 == 0,
                  "GraphicsDevice::writeRWStructuredBuffer", "stride %u must be a non-zero multiple of 4",
                  descriptor.stride);
    SLAG_VALIDATE(self->instance, descriptor.stride == 0 || descriptor.offset % descriptor.stride == 0,
                  "GraphicsDevice::writeRWStructuredBuffer", "offset must be a multiple of the stride");
    SLAG_VALIDATE(self->instance, descriptor.count <= kMaxStructuredBufferElements,
                  "GraphicsDevice::writeRWStructuredBuffer", "count %u exceeds kMaxStructuredBufferElements",
                  descriptor.count);
    SLAG_VALIDATE(self->instance,
                  !buffer || descriptor.offset + uint64_t(descriptor.count) * descriptor.stride <= buffer->size,
                  "GraphicsDevice::writeRWStructuredBuffer", "the view ends past the buffer");
    SLAG_DISPATCH(self, writeRWStructuredBuffer)(self, index, descriptor);
}

} // namespace slag
