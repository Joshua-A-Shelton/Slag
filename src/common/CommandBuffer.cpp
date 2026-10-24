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
    SLAG_VALIDATE(self->device->instance, offset + size <= limits::maxPushConstantsSize, "CommandBuffer::pushConstants",
                  "offset %u + size %u exceeds limits::maxPushConstantsSize (%u)", offset, size,
                  limits::maxPushConstantsSize);
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
                            std::span<const TextureBarrier> textureBarriers)
{
    CommandBufferCommon* self = common(this);
    SLAG_VALIDATE_RECORDING(self, "CommandBuffer::barrier");
#ifdef SLAG_VALIDATION
    InstanceCommon* instance = self->device->instance;
    for (const BufferBarrier& barrier : bufferBarriers)
    {
        SLAG_VALIDATE(instance, barrier.buffer, "CommandBuffer::barrier", "a buffer barrier has no buffer");
    }
    for (const TextureBarrier& barrier : textureBarriers)
    {
        auto* texture = static_cast<const TextureCommon*>(barrier.texture);
        SLAG_VALIDATE(instance, texture, "CommandBuffer::barrier", "a texture barrier has no texture");
        if (!texture)
        {
            continue;
        }
        const SubresourceRange& range = barrier.range;
        uint32_t layers = texture->type == TextureType::Texture3D ? 1 : texture->layerCount;
        SLAG_VALIDATE(instance,
                      range.baseMip < texture->mipCount && (range.mipCount == SubresourceRange::remainingMips ||
                                                            range.mipCount <= texture->mipCount - range.baseMip),
                      "CommandBuffer::barrier", "the mip range is outside the texture (%u mips)", texture->mipCount);
        SLAG_VALIDATE(instance,
                      range.baseLayer < layers && (range.layerCount == SubresourceRange::remainingLayers ||
                                                   range.layerCount <= layers - range.baseLayer),
                      "CommandBuffer::barrier", "the layer range is outside the texture (%u layers)", layers);
        bool depth = hasTraits(texture->format, FormatTraits::Depth);
        SLAG_VALIDATE(instance,
                      range.plane == TexturePlane::Color || (range.plane == TexturePlane::Depth && depth) ||
                          (range.plane == TexturePlane::Stencil && hasTraits(texture->format, FormatTraits::Stencil)),
                      "CommandBuffer::barrier", "the texture has no such plane");
        // Copy queues can't transition layouts; textures stay in Common (ADR 0049).
        SLAG_VALIDATE(instance,
                      self->queueType != CommandQueueType::Copy || (barrier.layoutBefore == TextureLayout::Common &&
                                                                    barrier.layoutAfter == TextureLayout::Common),
                      "CommandBuffer::barrier", "textures stay in the Common layout on copy queues");
    }
#endif
    if (globalBarriers.empty() && bufferBarriers.empty() && textureBarriers.empty())
    {
        return;
    }
    SLAG_DISPATCH(self, cmdBarrier)(self, globalBarriers, bufferBarriers, textureBarriers);
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

namespace {

#ifdef SLAG_VALIDATION
uint32_t mipSize(uint32_t size, uint32_t mip)
{
    return size >> mip ? size >> mip : 1;
}

// A texture region of a copy (ADR 0048).
void validateTextureRegion(InstanceCommon* instance, const TextureLocation& location, uint32_t width, uint32_t height,
                           uint32_t depth, const char* function)
{
    auto* texture = static_cast<const TextureCommon*>(location.texture);
    SLAG_VALIDATE(instance, texture, function, "a texture is null");
    if (!texture)
    {
        return;
    }
    SLAG_VALIDATE(instance, texture->sampleCount == 1, function, "MSAA textures can't be copied");
    SLAG_VALIDATE(instance, location.mip < texture->mipCount, function, "mip %u is outside the texture (%u mips)",
                  location.mip, texture->mipCount);
    uint32_t layers = texture->type == TextureType::Texture3D ? 1 : texture->layerCount;
    SLAG_VALIDATE(instance, location.layer < layers, function, "layer %u is outside the texture (%u layers)",
                  location.layer, layers);
    uint32_t mipWidth = mipSize(texture->width, location.mip);
    uint32_t mipHeight = mipSize(texture->height, location.mip);
    uint32_t mipDepth = texture->type == TextureType::Texture3D ? mipSize(texture->depth, location.mip) : 1;
    SLAG_VALIDATE(instance,
                  width >= 1 && height >= 1 && depth >= 1 && location.x + width <= mipWidth &&
                      location.y + height <= mipHeight && location.z + depth <= mipDepth,
                  function, "the region (%u,%u,%u)+(%ux%ux%u) is outside the mip (%ux%ux%u)", location.x, location.y,
                  location.z, width, height, depth, mipWidth, mipHeight, mipDepth);
    if (hasTraits(texture->format, FormatTraits::Compressed))
    {
        // Whole blocks, except where the region reaches the mip's edge.
        SLAG_VALIDATE(instance,
                      location.x % 4 == 0 && location.y % 4 == 0 &&
                          (width % 4 == 0 || location.x + width == mipWidth) &&
                          (height % 4 == 0 || location.y + height == mipHeight),
                      function, "block-compressed regions cover whole 4x4 blocks");
    }
    bool depthStencil = hasTraits(texture->format, FormatTraits::Depth);
    bool stencil = hasTraits(texture->format, FormatTraits::Stencil);
    SLAG_VALIDATE(instance,
                  (location.plane == TexturePlane::Color && !stencil) ||
                      (location.plane == TexturePlane::Depth && depthStencil) ||
                      (location.plane == TexturePlane::Stencil && stencil),
                  function, "the plane doesn't fit the format (depth/stencil textures copy one plane at a time)");
}

void validateBufferRegion(InstanceCommon* instance, const BufferLocation& location, const TextureLocation& texture,
                          uint32_t width, uint32_t height, uint32_t depth, const char* function)
{
    auto* buffer = static_cast<const BufferCommon*>(location.buffer);
    auto* textureObject = static_cast<const TextureCommon*>(texture.texture);
    SLAG_VALIDATE(instance, buffer, function, "the buffer is null");
    if (!buffer || !textureObject)
    {
        return;
    }
    SLAG_VALIDATE(instance, location.offset % alignment::textureCopyOffset == 0, function,
                  "the buffer offset %llu is not a multiple of %u", static_cast<unsigned long long>(location.offset),
                  alignment::textureCopyOffset);
    SLAG_VALIDATE(instance, location.rowPitch % alignment::textureCopyRowPitch == 0, function,
                  "the row pitch %u is not a multiple of %u", location.rowPitch, alignment::textureCopyRowPitch);
    uint32_t block = blockDimension(textureObject->format);
    uint64_t rowBytes = uint64_t((width + block - 1) / block) * copyBlockBytes(textureObject->format, texture.plane);
    uint64_t rows = (height + block - 1) / block;
    SLAG_VALIDATE(instance, location.rowPitch >= rowBytes, function, "the row pitch %u is smaller than a row (%llu)",
                  location.rowPitch, static_cast<unsigned long long>(rowBytes));
    uint64_t end = location.offset + uint64_t(location.rowPitch) * rows * depth;
    SLAG_VALIDATE(instance, end <= buffer->size, function, "the region ends past the buffer (%llu > %llu bytes)",
                  static_cast<unsigned long long>(end), static_cast<unsigned long long>(buffer->size));
}

#endif

} // namespace

void CommandBuffer::copyTexture(const TextureLocation& destination, const TextureLocation& source, uint32_t width,
                                uint32_t height, uint32_t depth)
{
    CommandBufferCommon* self = common(this);
    SLAG_VALIDATE_RECORDING(self, "CommandBuffer::copyTexture");
#ifdef SLAG_VALIDATION
    InstanceCommon* instance = self->device->instance;
    validateTextureRegion(instance, destination, width, height, depth, "CommandBuffer::copyTexture");
    validateTextureRegion(instance, source, width, height, depth, "CommandBuffer::copyTexture");
    auto* dst = static_cast<const TextureCommon*>(destination.texture);
    auto* src = static_cast<const TextureCommon*>(source.texture);
    SLAG_VALIDATE(instance, !dst || !src || dst->format == src->format || formatsCastable(dst->format, src->format),
                  "CommandBuffer::copyTexture", "%s and %s are neither the same nor castable",
                  dst ? formatInfo(dst->format).name : "", src ? formatInfo(src->format).name : "");
#endif
    SLAG_DISPATCH(self, cmdCopyTexture)(self, destination, source, width, height, depth);
}

void CommandBuffer::copyBufferToTexture(const TextureLocation& destination, const BufferLocation& source,
                                        uint32_t width, uint32_t height, uint32_t depth)
{
    CommandBufferCommon* self = common(this);
    SLAG_VALIDATE_RECORDING(self, "CommandBuffer::copyBufferToTexture");
#ifdef SLAG_VALIDATION
    validateTextureRegion(self->device->instance, destination, width, height, depth,
                          "CommandBuffer::copyBufferToTexture");
    validateBufferRegion(self->device->instance, source, destination, width, height, depth,
                         "CommandBuffer::copyBufferToTexture");
#endif
    SLAG_DISPATCH(self, cmdCopyBufferTexture)(self, true, destination, source, width, height, depth);
}

void CommandBuffer::copyTextureToBuffer(const BufferLocation& destination, const TextureLocation& source,
                                        uint32_t width, uint32_t height, uint32_t depth)
{
    CommandBufferCommon* self = common(this);
    SLAG_VALIDATE_RECORDING(self, "CommandBuffer::copyTextureToBuffer");
#ifdef SLAG_VALIDATION
    validateTextureRegion(self->device->instance, source, width, height, depth, "CommandBuffer::copyTextureToBuffer");
    validateBufferRegion(self->device->instance, destination, source, width, height, depth,
                         "CommandBuffer::copyTextureToBuffer");
#endif
    SLAG_DISPATCH(self, cmdCopyBufferTexture)(self, false, source, destination, width, height, depth);
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
    SLAG_VALIDATE(self->instance, descriptor.count <= limits::maxStructuredBufferElements,
                  "GraphicsDevice::writeRWStructuredBuffer", "count %u exceeds limits::maxStructuredBufferElements",
                  descriptor.count);
    SLAG_VALIDATE(self->instance,
                  !buffer || descriptor.offset + uint64_t(descriptor.count) * descriptor.stride <= buffer->size,
                  "GraphicsDevice::writeRWStructuredBuffer", "the view ends past the buffer");
    SLAG_DISPATCH(self, writeRWStructuredBuffer)(self, index, descriptor);
}

} // namespace slag
