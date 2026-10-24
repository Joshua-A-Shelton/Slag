#include "common/Logging.h"
#include "common/Objects.h"

#include <slag/CommandBuffer.h>
#include <slag/Limits.h>

namespace slag {

namespace {

CommandBufferCommon* common(CommandBuffer* commandBuffer)
{
    return static_cast<CommandBufferCommon*>(commandBuffer);
}

uint32_t mipSize(uint32_t size, uint32_t mip)
{
    return size >> mip ? size >> mip : 1;
}

void viewExtent(const AttachmentViewCommon& view, uint32_t& width, uint32_t& height)
{
    width = mipSize(view.texture->width, view.mip);
    height = mipSize(view.texture->height, view.mip);
}

} // namespace

#define SLAG_VALIDATE_IN_PASS(self, function)                                                                          \
    SLAG_VALIDATE((self)->device->instance, (self)->state == CommandBufferState::Recording && (self)->inRenderPass,    \
                  function, "not inside a render pass")

void CommandBuffer::beginRendering(const RenderingDesc& desc)
{
    SLAG_CHECK_LIVE_THIS(CommandBufferCommon, "CommandBuffer", "CommandBuffer::beginRendering");
    CommandBufferCommon* self = common(this);
    [[maybe_unused]] InstanceCommon* instance = self->device->instance;
    [[maybe_unused]] const char* function = "CommandBuffer::beginRendering";
    SLAG_VALIDATE(instance, self->state == CommandBufferState::Recording && !self->inRenderPass, function,
                  "not recording, or a render pass is already open");
    SLAG_VALIDATE(instance, self->queueType == CommandQueueType::Graphics, function,
                  "render passes need a graphics command buffer");
    SLAG_VALIDATE(instance, desc.colorAttachments.size() <= limits::maxColorTargets, function,
                  "%zu color attachments exceed limits::maxColorTargets", desc.colorAttachments.size());

    // The pass covers the attachments' shared extent (ADR 0046).
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t layers = 0;
    uint32_t samples = 0;
    self->passColorCount = static_cast<uint32_t>(desc.colorAttachments.size() < 8 ? desc.colorAttachments.size() : 8);
    auto consider = [&](const AttachmentViewCommon* view) {
        SLAG_VALIDATE_LIVE(instance, view, "AttachmentView", function);
        uint32_t viewWidth = 0;
        uint32_t viewHeight = 0;
        viewExtent(*view, viewWidth, viewHeight);
        if (layers == 0)
        {
            width = viewWidth;
            height = viewHeight;
            layers = view->layerCount;
            samples = view->texture->sampleCount;
        }
        SLAG_VALIDATE(instance, viewWidth == width && viewHeight == height && view->layerCount == layers, function,
                      "attachments differ in size or layer count");
        SLAG_VALIDATE(instance, view->texture->sampleCount == samples, function, "attachments differ in sample count");
    };
    for (uint32_t i = 0; i < self->passColorCount; ++i)
    {
        const ColorAttachment& attachment = desc.colorAttachments[i];
        auto* view = static_cast<const AttachmentViewCommon*>(attachment.view);
        self->passColorFormats[i] = view ? view->format : PixelFormat::Undefined;
        if (!view)
        {
            continue;
        }
        SLAG_VALIDATE(instance, (view->texture->usage & TextureUsage::RenderTarget) != TextureUsage::None, function,
                      "color attachment %u is not a render target view", i);
        consider(view);
    }
    self->passDepthStencilFormat = PixelFormat::Undefined;
    self->passReadOnlyDepth = false;
    self->passReadOnlyStencil = false;
    if (desc.depthStencilAttachment && desc.depthStencilAttachment->view)
    {
        const DepthStencilAttachment& attachment = *desc.depthStencilAttachment;
        auto* view = static_cast<const AttachmentViewCommon*>(attachment.view);
        SLAG_VALIDATE(instance, (view->texture->usage & TextureUsage::DepthStencil) != TextureUsage::None, function,
                      "the depth/stencil attachment is not a depth/stencil view");
        consider(view);
        self->passDepthStencilFormat = view->format;
        self->passReadOnlyDepth = attachment.readOnlyDepth;
        self->passReadOnlyStencil = attachment.readOnlyStencil;
    }
    SLAG_VALIDATE(instance, layers > 0, function, "a render pass needs at least one attachment");
    self->passSampleCount = samples ? samples : 1;
    self->inRenderPass = true;
    if (!self->recordedPass)
    {
        self->resumesPass = desc.resuming;
    }
    self->recordedPass = true;
    self->suspendsPass = desc.suspending;
    if (layers == 0)
    {
        return;
    }
    SLAG_DISPATCH(self, cmdBeginRendering)(self, desc, width, height, layers);
}

void CommandBuffer::endRendering()
{
    SLAG_CHECK_LIVE_THIS(CommandBufferCommon, "CommandBuffer", "CommandBuffer::endRendering");
    CommandBufferCommon* self = common(this);
    SLAG_VALIDATE_IN_PASS(self, "CommandBuffer::endRendering");
    self->inRenderPass = false;
    SLAG_DISPATCH(self, cmdEndRendering)(self);
}

void CommandBuffer::bindVertexBuffers(uint32_t firstSlot, std::span<const VertexBufferBinding> bindings)
{
    SLAG_CHECK_LIVE_THIS(CommandBufferCommon, "CommandBuffer", "CommandBuffer::bindVertexBuffers");
    CommandBufferCommon* self = common(this);
    SLAG_VALIDATE(self->device->instance, self->state == CommandBufferState::Recording,
                  "CommandBuffer::bindVertexBuffers", "not recording");
    SLAG_VALIDATE(self->device->instance, firstSlot + bindings.size() <= limits::maxVertexBufferSlots,
                  "CommandBuffer::bindVertexBuffers", "slots beyond limits::maxVertexBufferSlots");
#ifdef SLAG_VALIDATION
    for (const VertexBufferBinding& binding : bindings)
    {
        auto* buffer = static_cast<const BufferCommon*>(binding.buffer);
        SLAG_VALIDATE_LIVE(self->device->instance, buffer, "Buffer", "CommandBuffer::bindVertexBuffers");
        SLAG_VALIDATE(self->device->instance, buffer && binding.offset + binding.size <= buffer->size,
                      "CommandBuffer::bindVertexBuffers", "a binding is null or ends past its buffer");
    }
#endif
    SLAG_DISPATCH(self, cmdBindVertexBuffers)(self, firstSlot, bindings);
}

void CommandBuffer::bindIndexBuffer(Buffer* buffer, uint64_t offset, uint64_t size, IndexFormat format)
{
    SLAG_CHECK_LIVE_THIS(CommandBufferCommon, "CommandBuffer", "CommandBuffer::bindIndexBuffer");
    SLAG_CHECK_LIVE(static_cast<const CommandBufferCommon*>(this)->device->instance, BufferCommon, buffer, "Buffer",
                    "CommandBuffer::bindIndexBuffer");
    CommandBufferCommon* self = common(this);
    auto* object = static_cast<BufferCommon*>(buffer);
    SLAG_VALIDATE(self->device->instance, self->state == CommandBufferState::Recording,
                  "CommandBuffer::bindIndexBuffer", "not recording");
    SLAG_VALIDATE(self->device->instance, object && offset + size <= object->size, "CommandBuffer::bindIndexBuffer",
                  "the buffer is null or the range ends past it");
    SLAG_VALIDATE(self->device->instance, offset % (format == IndexFormat::Uint16 ? 2 : 4) == 0,
                  "CommandBuffer::bindIndexBuffer", "the offset isn't a multiple of the index size");
    SLAG_DISPATCH(self, cmdBindIndexBuffer)(self, object, offset, size, format);
}

void CommandBuffer::setViewports(std::span<const Viewport> viewports)
{
    SLAG_CHECK_LIVE_THIS(CommandBufferCommon, "CommandBuffer", "CommandBuffer::setViewports");
    CommandBufferCommon* self = common(this);
    SLAG_VALIDATE(self->device->instance, !viewports.empty() && viewports.size() <= limits::maxViewports,
                  "CommandBuffer::setViewports", "1 to %u viewports", limits::maxViewports);
    SLAG_DISPATCH(self, cmdSetViewports)(self, viewports);
}

void CommandBuffer::setScissors(std::span<const Rect> scissors)
{
    SLAG_CHECK_LIVE_THIS(CommandBufferCommon, "CommandBuffer", "CommandBuffer::setScissors");
    CommandBufferCommon* self = common(this);
    SLAG_VALIDATE(self->device->instance, !scissors.empty() && scissors.size() <= limits::maxViewports,
                  "CommandBuffer::setScissors", "1 to %u scissors", limits::maxViewports);
    SLAG_DISPATCH(self, cmdSetScissors)(self, scissors);
}

void CommandBuffer::setBlendConstant(const float constant[4])
{
    SLAG_CHECK_LIVE_THIS(CommandBufferCommon, "CommandBuffer", "CommandBuffer::setBlendConstant");
    CommandBufferCommon* self = common(this);
    SLAG_DISPATCH(self, cmdSetBlendConstant)(self, constant);
}

void CommandBuffer::setStencilReference(uint8_t reference)
{
    SLAG_CHECK_LIVE_THIS(CommandBufferCommon, "CommandBuffer", "CommandBuffer::setStencilReference");
    CommandBufferCommon* self = common(this);
    SLAG_DISPATCH(self, cmdSetStencilReference)(self, reference);
}

void CommandBuffer::setDepthBounds(float minDepth, float maxDepth)
{
    SLAG_CHECK_LIVE_THIS(CommandBufferCommon, "CommandBuffer", "CommandBuffer::setDepthBounds");
    CommandBufferCommon* self = common(this);
    SLAG_VALIDATE(self->device->instance, self->device->capabilities.depthBounds, "CommandBuffer::setDepthBounds",
                  "the depth bounds test is not supported");
    SLAG_VALIDATE(self->device->instance, 0.0f <= minDepth && minDepth <= maxDepth && maxDepth <= 1.0f,
                  "CommandBuffer::setDepthBounds", "bounds must satisfy 0 <= min <= max <= 1");
    SLAG_DISPATCH(self, cmdSetDepthBounds)(self, minDepth, maxDepth);
}

namespace {

// The bound pipeline fits the open pass (ADRs 0040, 0042).
[[maybe_unused]] void validateDraw(CommandBufferCommon* self, const char* function)
{
#ifdef SLAG_VALIDATION
    InstanceCommon* instance = self->device->instance;
    SLAG_VALIDATE_IN_PASS(self, function);
    const PipelineCommon* pipeline = self->boundPipeline;
    SLAG_VALIDATE(instance, pipeline && pipeline->kind == PipelineKind::Graphics, function,
                  "no graphics pipeline is bound");
    if (!pipeline || pipeline->kind != PipelineKind::Graphics)
    {
        return;
    }
    bool formatsMatch =
        pipeline->depthStencilFormat == self->passDepthStencilFormat && pipeline->sampleCount == self->passSampleCount;
    uint32_t count = pipeline->colorCount > self->passColorCount ? pipeline->colorCount : self->passColorCount;
    for (uint32_t i = 0; i < count; ++i)
    {
        PixelFormat a = i < pipeline->colorCount ? pipeline->colorFormats[i] : PixelFormat::Undefined;
        PixelFormat b = i < self->passColorCount ? self->passColorFormats[i] : PixelFormat::Undefined;
        formatsMatch = formatsMatch && a == b;
    }
    SLAG_VALIDATE(instance, formatsMatch, function,
                  "the pipeline's target formats or sample count don't match the render pass");
    SLAG_VALIDATE(instance, !(pipeline->writesDepth && self->passReadOnlyDepth), function,
                  "the pipeline writes depth, but the pass's depth is read-only");
    SLAG_VALIDATE(instance, !(pipeline->writesStencil && self->passReadOnlyStencil), function,
                  "the pipeline writes stencil, but the pass's stencil is read-only");
#else
    (void)self;
    (void)function;
#endif
}

} // namespace

void CommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    SLAG_CHECK_LIVE_THIS(CommandBufferCommon, "CommandBuffer", "CommandBuffer::draw");
    CommandBufferCommon* self = common(this);
    validateDraw(self, "CommandBuffer::draw");
    SLAG_DISPATCH(self, cmdDraw)(self, vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
                                uint32_t firstInstance)
{
    SLAG_CHECK_LIVE_THIS(CommandBufferCommon, "CommandBuffer", "CommandBuffer::drawIndexed");
    CommandBufferCommon* self = common(this);
    validateDraw(self, "CommandBuffer::drawIndexed");
    SLAG_DISPATCH(self, cmdDrawIndexed)(self, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

namespace {

[[maybe_unused]] void validateIndirect(CommandBufferCommon* self, IndirectKind kind, const BufferCommon* arguments,
                                       uint64_t offset, uint32_t maxCount, const BufferCommon* count,
                                       uint64_t countOffset, const char* function)
{
#ifdef SLAG_VALIDATION
    InstanceCommon* instance = self->device->instance;
    uint64_t stride = kind == IndirectKind::Draw          ? sizeof(DrawArguments)
                      : kind == IndirectKind::DrawIndexed ? sizeof(DrawIndexedArguments)
                                                          : sizeof(DispatchArguments);
    SLAG_VALIDATE(instance, arguments && offset % 4 == 0 && offset + stride * maxCount <= arguments->size, function,
                  "the argument buffer is null, misaligned, or too small for %u commands", maxCount);
    SLAG_VALIDATE(instance, !count || (countOffset % 4 == 0 && countOffset + 4 <= count->size), function,
                  "the count buffer offset is misaligned or past the buffer");
#else
    (void)self;
    (void)kind;
    (void)arguments;
    (void)offset;
    (void)maxCount;
    (void)count;
    (void)countOffset;
    (void)function;
#endif
}

void indirect(CommandBuffer* object, IndirectKind kind, Buffer* arguments, uint64_t offset, uint32_t maxCount,
              Buffer* count, uint64_t countOffset, const char* function)
{
    CommandBufferCommon* self = common(object);
    auto* argumentBuffer = static_cast<BufferCommon*>(arguments);
    auto* countBuffer = static_cast<BufferCommon*>(count);
    if (kind == IndirectKind::Dispatch)
    {
        SLAG_VALIDATE(self->device->instance, self->state == CommandBufferState::Recording && !self->inRenderPass,
                      function, "not recording, or inside a render pass");
        SLAG_VALIDATE(self->device->instance, self->boundPipeline && self->boundPipeline->kind == PipelineKind::Compute,
                      function, "no compute pipeline is bound");
    }
    else
    {
        validateDraw(self, function);
    }
    validateIndirect(self, kind, argumentBuffer, offset, maxCount, countBuffer, countOffset, function);
    SLAG_DISPATCH(self, cmdIndirect)(self, kind, argumentBuffer, offset, maxCount, countBuffer, countOffset);
}

} // namespace

void CommandBuffer::drawIndirect(Buffer* argumentBuffer, uint64_t argumentOffset, uint32_t drawCount)
{
    SLAG_CHECK_LIVE_THIS(CommandBufferCommon, "CommandBuffer", "CommandBuffer::drawIndirect");
    SLAG_CHECK_LIVE(static_cast<const CommandBufferCommon*>(this)->device->instance, BufferCommon, argumentBuffer,
                    "Buffer", "CommandBuffer::drawIndirect");
    indirect(this, IndirectKind::Draw, argumentBuffer, argumentOffset, drawCount, nullptr, 0,
             "CommandBuffer::drawIndirect");
}

void CommandBuffer::drawIndirectCount(Buffer* argumentBuffer, uint64_t argumentOffset, Buffer* countBuffer,
                                      uint64_t countOffset, uint32_t maxDrawCount)
{
    SLAG_CHECK_LIVE_THIS(CommandBufferCommon, "CommandBuffer", "CommandBuffer::drawIndirectCount");
    SLAG_CHECK_LIVE(static_cast<const CommandBufferCommon*>(this)->device->instance, BufferCommon, argumentBuffer,
                    "Buffer", "CommandBuffer::drawIndirectCount");
    SLAG_CHECK_LIVE(static_cast<const CommandBufferCommon*>(this)->device->instance, BufferCommon, countBuffer,
                    "Buffer", "CommandBuffer::drawIndirectCount");
    SLAG_VALIDATE(common(this)->device->instance, countBuffer, "CommandBuffer::drawIndirectCount",
                  "the count buffer is null");
    indirect(this, IndirectKind::Draw, argumentBuffer, argumentOffset, maxDrawCount, countBuffer, countOffset,
             "CommandBuffer::drawIndirectCount");
}

void CommandBuffer::drawIndexedIndirect(Buffer* argumentBuffer, uint64_t argumentOffset, uint32_t drawCount)
{
    SLAG_CHECK_LIVE_THIS(CommandBufferCommon, "CommandBuffer", "CommandBuffer::drawIndexedIndirect");
    SLAG_CHECK_LIVE(static_cast<const CommandBufferCommon*>(this)->device->instance, BufferCommon, argumentBuffer,
                    "Buffer", "CommandBuffer::drawIndexedIndirect");
    indirect(this, IndirectKind::DrawIndexed, argumentBuffer, argumentOffset, drawCount, nullptr, 0,
             "CommandBuffer::drawIndexedIndirect");
}

void CommandBuffer::drawIndexedIndirectCount(Buffer* argumentBuffer, uint64_t argumentOffset, Buffer* countBuffer,
                                             uint64_t countOffset, uint32_t maxDrawCount)
{
    SLAG_CHECK_LIVE_THIS(CommandBufferCommon, "CommandBuffer", "CommandBuffer::drawIndexedIndirectCount");
    SLAG_CHECK_LIVE(static_cast<const CommandBufferCommon*>(this)->device->instance, BufferCommon, argumentBuffer,
                    "Buffer", "CommandBuffer::drawIndexedIndirectCount");
    SLAG_CHECK_LIVE(static_cast<const CommandBufferCommon*>(this)->device->instance, BufferCommon, countBuffer,
                    "Buffer", "CommandBuffer::drawIndexedIndirectCount");
    SLAG_VALIDATE(common(this)->device->instance, countBuffer, "CommandBuffer::drawIndexedIndirectCount",
                  "the count buffer is null");
    indirect(this, IndirectKind::DrawIndexed, argumentBuffer, argumentOffset, maxDrawCount, countBuffer, countOffset,
             "CommandBuffer::drawIndexedIndirectCount");
}

void CommandBuffer::dispatchIndirect(Buffer* argumentBuffer, uint64_t argumentOffset)
{
    SLAG_CHECK_LIVE_THIS(CommandBufferCommon, "CommandBuffer", "CommandBuffer::dispatchIndirect");
    SLAG_CHECK_LIVE(static_cast<const CommandBufferCommon*>(this)->device->instance, BufferCommon, argumentBuffer,
                    "Buffer", "CommandBuffer::dispatchIndirect");
    indirect(this, IndirectKind::Dispatch, argumentBuffer, argumentOffset, 1, nullptr, 0,
             "CommandBuffer::dispatchIndirect");
}

} // namespace slag
