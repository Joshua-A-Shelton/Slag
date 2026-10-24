#include "common/Logging.h"
#include "common/Objects.h"

#include <slag/CommandBuffer.h>
#include <slag/GraphicsDevice.h>
#include <slag/Limits.h>
#include <slag/Pipeline.h>

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

// Barriers, copies, and dispatches stay outside render passes (ADR 0046).
#define SLAG_VALIDATE_OUTSIDE_PASS(self, function)                                                                     \
    SLAG_VALIDATE((self)->device->instance, !(self)->inRenderPass, function, "not allowed inside a render pass")

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
        self->inRenderPass = false;
        self->resumesPass = false;
        self->suspendsPass = false;
        self->recordedPass = false;
    }
    return result;
}

Result<void> CommandBuffer::end()
{
    CommandBufferCommon* self = common(this);
    SLAG_VALIDATE_RECORDING(self, "CommandBuffer::end");
    SLAG_VALIDATE(self->device->instance, !self->inRenderPass, "CommandBuffer::end", "a render pass is still open");
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

void CommandBuffer::setConstantBuffer(uint64_t address, uint32_t size)
{
    CommandBufferCommon* self = common(this);
    SLAG_VALIDATE_RECORDING(self, "CommandBuffer::setConstantBuffer");
    SLAG_VALIDATE(self->device->instance, self->queueType != CommandQueueType::Copy, "CommandBuffer::setConstantBuffer",
                  "copy command buffers have no shader state");
    SLAG_VALIDATE(self->device->instance, address % alignment::constantBuffer == 0, "CommandBuffer::setConstantBuffer",
                  "the address is not a multiple of %u", alignment::constantBuffer);
    SLAG_VALIDATE(self->device->instance,
                  size > 0 && size % alignment::constantBuffer == 0 && size <= limits::maxConstantBufferSize,
                  "CommandBuffer::setConstantBuffer", "size %u is not a multiple of %u up to %u", size,
                  alignment::constantBuffer, limits::maxConstantBufferSize);
    SLAG_DISPATCH(self, cmdSetConstantBuffer)(self, address, size);
}

void CommandBuffer::dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    CommandBufferCommon* self = common(this);
    SLAG_VALIDATE_RECORDING(self, "CommandBuffer::dispatch");
    SLAG_VALIDATE_OUTSIDE_PASS(self, "CommandBuffer::dispatch");
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
    SLAG_VALIDATE_OUTSIDE_PASS(self, "CommandBuffer::barrier");
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
    SLAG_VALIDATE_OUTSIDE_PASS(self, "CommandBuffer::copyBuffer");
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
    SLAG_VALIDATE_OUTSIDE_PASS(self, "CommandBuffer::copyTexture");
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

void CommandBuffer::resolveTexture(Texture* destination, uint32_t destinationMip, uint32_t destinationLayer,
                                   Texture* source, uint32_t sourceLayer)
{
    CommandBufferCommon* self = common(this);
    auto* dst = static_cast<TextureCommon*>(destination);
    auto* src = static_cast<TextureCommon*>(source);
    [[maybe_unused]] const char* function = "CommandBuffer::resolveTexture";
    SLAG_VALIDATE_RECORDING(self, function);
    SLAG_VALIDATE_OUTSIDE_PASS(self, function);
#ifdef SLAG_VALIDATION
    InstanceCommon* instance = self->device->instance;
    SLAG_VALIDATE(instance, self->queueType == CommandQueueType::Graphics, function, "resolves need a graphics queue");
    SLAG_VALIDATE(instance, dst && src, function, "a texture is null");
    if (dst && src)
    {
        SLAG_VALIDATE(instance, src->sampleCount > 1 && dst->sampleCount == 1, function,
                      "resolves go from an MSAA texture into a single-sampled one");
        SLAG_VALIDATE(instance, dst->format == src->format || formatsCastable(dst->format, src->format), function,
                      "the formats differ");
        SLAG_VALIDATE(instance, !hasTraits(src->format, FormatTraits::Integer | FormatTraits::Depth), function,
                      "integer and depth formats can't be resolved");
        SLAG_VALIDATE(instance,
                      destinationMip < dst->mipCount && destinationLayer < dst->layerCount &&
                          sourceLayer < src->layerCount,
                      function, "the mip or a layer is outside its texture");
        SLAG_VALIDATE(instance,
                      (dst->width >> destinationMip ? dst->width >> destinationMip : 1) == src->width &&
                          (dst->height >> destinationMip ? dst->height >> destinationMip : 1) == src->height,
                      function, "the destination mip and the source differ in size");
    }
#endif
    SLAG_DISPATCH(self, cmdResolveTexture)(self, dst, destinationMip, destinationLayer, src, sourceLayer);
}

void CommandBuffer::copyBufferToTexture(const TextureLocation& destination, const BufferLocation& source,
                                        uint32_t width, uint32_t height, uint32_t depth)
{
    CommandBufferCommon* self = common(this);
    SLAG_VALIDATE_RECORDING(self, "CommandBuffer::copyBufferToTexture");
    SLAG_VALIDATE_OUTSIDE_PASS(self, "CommandBuffer::copyBufferToTexture");
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
    SLAG_VALIDATE_OUTSIDE_PASS(self, "CommandBuffer::copyTextureToBuffer");
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
    // A suspended render pass resumes in the very next command buffer (ADR 0046).
    for (size_t i = 0; i < desc.commandBuffers.size(); ++i)
    {
        CommandBufferCommon* object = common(desc.commandBuffers[i]);
        bool nextResumes = i + 1 < desc.commandBuffers.size() && common(desc.commandBuffers[i + 1])->resumesPass;
        bool previousSuspends = i > 0 && common(desc.commandBuffers[i - 1])->suspendsPass;
        SLAG_VALIDATE(self->device->instance, !object->suspendsPass || nextResumes, "CommandQueue::submit",
                      "command buffer %zu suspends a render pass the next one doesn't resume", i);
        SLAG_VALIDATE(self->device->instance, !object->resumesPass || previousSuspends, "CommandQueue::submit",
                      "command buffer %zu resumes a render pass the previous one didn't suspend", i);
    }
#endif
    if (self->device->lost)
    {
        return Status::DeviceLost;
    }
    return SLAG_DISPATCH(self, submit)(self, desc);
}

// --- Pipelines (GraphicsDevice, Pipeline) -----------------------------------

namespace {

#ifdef SLAG_VALIDATION
bool isDualSource(BlendFactor factor)
{
    return factor == BlendFactor::Src1Color || factor == BlendFactor::InvSrc1Color ||
           factor == BlendFactor::Src1Alpha || factor == BlendFactor::InvSrc1Alpha;
}

// D3D12 rejects color factors in the alpha blend.
bool isColorFactor(BlendFactor factor)
{
    return factor == BlendFactor::SrcColor || factor == BlendFactor::InvSrcColor || factor == BlendFactor::DestColor ||
           factor == BlendFactor::InvDestColor || factor == BlendFactor::Src1Color ||
           factor == BlendFactor::InvSrc1Color;
}

bool supports(const GraphicsDeviceCommon* device, PixelFormat format, FormatSupport flag)
{
    return (device->formatSupport[static_cast<size_t>(format)] & flag) != FormatSupport::None;
}

FormatSupport multisampleFlag(uint32_t sampleCount)
{
    return sampleCount == 2   ? FormatSupport::Multisample2
           : sampleCount == 4 ? FormatSupport::Multisample4
           : sampleCount == 8 ? FormatSupport::Multisample8
                              : FormatSupport::None;
}

// ADRs 0037-0042.
void validateGraphicsPipeline(GraphicsDeviceCommon* self, const GraphicsPipelineDesc& desc)
{
    InstanceCommon* instance = self->instance;
    const char* function = "GraphicsDevice::createGraphicsPipeline";
    SLAG_VALIDATE(instance, !desc.vertex.binaries.empty(), function, "no vertex shader binaries");
    SLAG_VALIDATE(instance, desc.colorFormats.size() <= limits::maxColorTargets, function,
                  "%zu color targets exceed limits::maxColorTargets", desc.colorFormats.size());
    SLAG_VALIDATE(instance, desc.sampleCount == 1 || multisampleFlag(desc.sampleCount) != FormatSupport::None, function,
                  "sample count %u is not 1, 2, 4, or 8", desc.sampleCount);
    FormatSupport msaa = multisampleFlag(desc.sampleCount);
    for (size_t i = 0; i < desc.colorFormats.size() && i < 8; ++i)
    {
        PixelFormat format = desc.colorFormats[i];
        const ColorTargetBlend& blend = desc.blend.targets[i];
        if (format == PixelFormat::Undefined)
        {
            continue;
        }
        SLAG_VALIDATE(instance, supports(self, format, FormatSupport::RenderTarget), function,
                      "color target %zu: %s can't be a render target", i, formatInfo(format).name);
        SLAG_VALIDATE(instance, !blend.blendEnable || supports(self, format, FormatSupport::Blend), function,
                      "color target %zu: %s doesn't support blending", i, formatInfo(format).name);
        SLAG_VALIDATE(instance, msaa == FormatSupport::None || supports(self, format, msaa), function,
                      "color target %zu: %s doesn't support %ux MSAA", i, formatInfo(format).name, desc.sampleCount);
        SLAG_VALIDATE(instance,
                      i == 0 || !blend.blendEnable ||
                          !(isDualSource(blend.srcColor) || isDualSource(blend.destColor) ||
                            isDualSource(blend.srcAlpha) || isDualSource(blend.destAlpha)),
                      function, "dual-source blending is for render target 0 only");
        SLAG_VALIDATE(instance,
                      !blend.blendEnable || (!isColorFactor(blend.srcAlpha) && !isColorFactor(blend.destAlpha)),
                      function, "color target %zu: alpha blend factors can't be color factors", i);
    }
    bool hasDepthStencil = desc.depthStencilFormat != PixelFormat::Undefined;
    SLAG_VALIDATE(instance, !hasDepthStencil || supports(self, desc.depthStencilFormat, FormatSupport::DepthStencil),
                  function, "%s can't be a depth/stencil target", formatInfo(desc.depthStencilFormat).name);
    SLAG_VALIDATE(instance,
                  !hasDepthStencil || msaa == FormatSupport::None || supports(self, desc.depthStencilFormat, msaa),
                  function, "%s doesn't support %ux MSAA", formatInfo(desc.depthStencilFormat).name, desc.sampleCount);
    SLAG_VALIDATE(instance, hasDepthStencil || (!desc.depthStencil.depthTest && !desc.depthStencil.stencilTest),
                  function, "depth or stencil testing without a depth/stencil format");
    SLAG_VALIDATE(instance, !desc.depthStencil.stencilTest || hasTraits(desc.depthStencilFormat, FormatTraits::Stencil),
                  function, "stencil testing needs a format with stencil");
    SLAG_VALIDATE(instance, !desc.depthStencil.depthBoundsTest || self->capabilities.depthBounds, function,
                  "the depth bounds test is not supported on \"%s\"", self->info.name);
    SLAG_VALIDATE(instance, !desc.rasterizer.conservativeRasterization || self->capabilities.conservativeRasterization,
                  function, "conservative rasterization is not supported on \"%s\"", self->info.name);

    const VertexInputLayout& input = desc.vertexInput;
    SLAG_VALIDATE(instance, input.attributes.size() <= limits::maxVertexAttributes, function,
                  "%zu vertex attributes exceed limits::maxVertexAttributes", input.attributes.size());
    SLAG_VALIDATE(instance, input.slots.size() <= limits::maxVertexBufferSlots, function,
                  "%zu vertex buffer slots exceed limits::maxVertexBufferSlots", input.slots.size());
    for (size_t i = 0; i < input.attributes.size(); ++i)
    {
        const VertexAttribute& attribute = input.attributes[i];
        SLAG_VALIDATE(instance, attribute.slot < input.slots.size(), function,
                      "attribute at location %u uses slot %u, but only %zu slots are declared", attribute.location,
                      attribute.slot, input.slots.size());
        SLAG_VALIDATE(instance, supports(self, attribute.format, FormatSupport::VertexBuffer), function,
                      "attribute at location %u: %s is not a vertex format", attribute.location,
                      formatInfo(attribute.format).name);
        for (size_t j = 0; j < i; ++j)
        {
            SLAG_VALIDATE(instance, input.attributes[j].location != attribute.location, function,
                          "two attributes use location %u", attribute.location);
        }
    }
}
#endif

} // namespace

Result<Pipeline*> GraphicsDevice::createGraphicsPipeline(const GraphicsPipelineDesc& desc)
{
    auto* self = static_cast<GraphicsDeviceCommon*>(this);
    SLAG_VALIDATE(self->instance, self->initialized, "GraphicsDevice::createGraphicsPipeline",
                  "device \"%s\" is not initialized", self->info.name);
#ifdef SLAG_VALIDATION
    validateGraphicsPipeline(self, desc);
#endif
    if (self->lost)
    {
        return Status::DeviceLost;
    }
    Result<PipelineCommon*> result = SLAG_DISPATCH(self, createGraphicsPipeline)(self, desc);
    if (!result)
    {
        return result.status();
    }
    PipelineCommon* pipeline = result.value();
    pipeline->userData = desc.userData;
    pipeline->kind = PipelineKind::Graphics;
    pipeline->colorCount = static_cast<uint32_t>(desc.colorFormats.size() < 8 ? desc.colorFormats.size() : 8);
    for (uint32_t i = 0; i < pipeline->colorCount; ++i)
    {
        pipeline->colorFormats[i] = desc.colorFormats[i];
    }
    pipeline->depthStencilFormat = desc.depthStencilFormat;
    pipeline->sampleCount = desc.sampleCount;
    pipeline->writesDepth = desc.depthStencil.depthTest && desc.depthStencil.depthWrite;
    pipeline->writesStencil = desc.depthStencil.stencilTest && desc.depthStencil.stencilWriteMask != 0;
    pipeline->vertexSlotCount = static_cast<uint32_t>(desc.vertexInput.slots.size());
    return static_cast<Pipeline*>(pipeline);
}

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

} // namespace slag
