// Validation of render passes, draws, and what may happen inside a pass.

#include "ValidationFixture.h"

#include <vector>

using namespace slag;

namespace {

// A recording graphics command buffer with an open pass on a 64x64 target.
struct OpenPass
{
    ValidationFixture& f;
    Texture* target = nullptr;
    AttachmentView* view = nullptr;
    CommandBuffer* commandBuffer = nullptr;

    explicit OpenPass(ValidationFixture& fixture)
        : f(fixture)
    {
        target = f.texture(TextureUsage::RenderTarget);
        view = f.device->createAttachmentView({.texture = target}).value();
        commandBuffer = f.recording();
        const ColorAttachment color = {.view = view};
        commandBuffer->beginRendering({.colorAttachments = {&color, 1}});
    }
};

} // namespace

TEST_CASE("render passes")
{
    ValidationFixture f;
    Texture* target = f.texture(TextureUsage::RenderTarget);
    AttachmentView* view = f.device->createAttachmentView({.texture = target}).value();
    const ColorAttachment color = {.view = view};
    CommandBuffer* commandBuffer = f.recording();
    CommandBuffer* compute = f.recording(CommandQueueType::Compute);
    CHECK_VALIDATION(f, "render passes need a graphics command buffer",
                     compute->beginRendering({.colorAttachments = {&color, 1}}));
    ColorAttachment nine[9];
    for (ColorAttachment& attachment : nine)
    {
        attachment.view = view;
    }
    CHECK_VALIDATION(f, "9 color attachments exceed", commandBuffer->beginRendering({.colorAttachments = nine}));
    commandBuffer->endRendering();
    Texture* bigger = f.texture(TextureUsage::RenderTarget, 128);
    AttachmentView* biggerView = f.device->createAttachmentView({.texture = bigger}).value();
    const ColorAttachment two[] = {{.view = view}, {.view = biggerView}};
    CHECK_VALIDATION(f, "attachments differ in size or layer count",
                     commandBuffer->beginRendering({.colorAttachments = two}));
    commandBuffer->endRendering();
    Texture* msaa = f.texture(TextureUsage::RenderTarget, 64, PixelFormat::R8G8B8A8Unorm, 4);
    AttachmentView* msaaView = f.device->createAttachmentView({.texture = msaa}).value();
    const ColorAttachment mixed[] = {{.view = view}, {.view = msaaView}};
    CHECK_VALIDATION(f, "attachments differ in sample count",
                     commandBuffer->beginRendering({.colorAttachments = mixed}));
    commandBuffer->endRendering();
    Texture* depth = f.texture(TextureUsage::DepthStencil, 64, PixelFormat::D32Float);
    AttachmentView* depthView = f.device->createAttachmentView({.texture = depth}).value();
    const ColorAttachment depthAsColor = {.view = depthView};
    CHECK_VALIDATION(f, "is not a render target view",
                     commandBuffer->beginRendering({.colorAttachments = {&depthAsColor, 1}}));
    commandBuffer->endRendering();
    const DepthStencilAttachment colorAsDepth = {.view = view};
    CHECK_VALIDATION(f, "is not a depth/stencil view",
                     commandBuffer->beginRendering({.depthStencilAttachment = &colorAsDepth}));
    commandBuffer->endRendering();
    CHECK_VALIDATION(f, "a render pass needs at least one attachment", commandBuffer->beginRendering({}));
    commandBuffer->endRendering();
    commandBuffer->beginRendering({.colorAttachments = {&color, 1}});
    CHECK_VALIDATION(f, "a render pass is already open",
                     commandBuffer->beginRendering({.colorAttachments = {&color, 1}}));
    CHECK_VALIDATION(f, "a render pass is still open", (void)commandBuffer->end());
    commandBuffer->endRendering();
    (void)commandBuffer->reset(); // end() went ahead
    (void)commandBuffer->begin();
    CHECK_VALIDATION(f, "not inside a render pass", commandBuffer->endRendering());

    f.device->destroy(biggerView);
    CHECK_VALIDATION(f, "use of a destroyed AttachmentView",
                     commandBuffer->beginRendering({.colorAttachments = {&two[1], 1}}));
    commandBuffer->endRendering();
    f.device->destroy(msaaView);
    f.device->destroy(depthView);
}

TEST_CASE("work that stays outside render passes")
{
    ValidationFixture f;
    OpenPass pass(f);
    CommandBuffer* commandBuffer = pass.commandBuffer;
    Buffer* a = f.buffer(1024);
    Buffer* b = f.buffer(1024);
    Texture* texture = f.texture();
    Texture* texture2 = f.texture();
    commandBuffer->bindPipeline(f.computePipeline());
    CHECK_VALIDATION(f, "not allowed inside a render pass", commandBuffer->dispatch(1, 1, 1));
    CHECK_VALIDATION(f, "not allowed inside a render pass", commandBuffer->barrier({}, {}, {}));
    CHECK_VALIDATION(f, "not allowed inside a render pass", commandBuffer->copyBuffer(a, 0, b, 0, 16));
    CHECK_VALIDATION(f, "not allowed inside a render pass",
                     commandBuffer->copyTexture({.texture = texture}, {.texture = texture2}, 4, 4, 1));
    CHECK_VALIDATION(f, "not allowed inside a render pass",
                     commandBuffer->copyBufferToTexture({.texture = texture}, {b, 0, 256}, 1, 1));
    CHECK_VALIDATION(f, "not allowed inside a render pass",
                     commandBuffer->copyTextureToBuffer({b, 0, 256}, {.texture = texture}, 1, 1));
    Texture* msaa = f.texture(TextureUsage::RenderTarget, 64, PixelFormat::R8G8B8A8Unorm, 4);
    CHECK_VALIDATION(f, "not allowed inside a render pass", commandBuffer->resolveTexture(pass.target, 0, 0, msaa, 0));
    CHECK_VALIDATION(f, "not recording, or inside a render pass", commandBuffer->dispatchIndirect(a, 0));

    // And on a command buffer that isn't recording.
    CommandBuffer* idle = f.device->createCommandBuffer({}).value();
    CHECK_VALIDATION(f, "not recording", idle->barrier({}, {}, {}));
    CHECK_VALIDATION(f, "not recording", idle->copyBuffer(a, 0, b, 0, 16));
    CHECK_VALIDATION(f, "not recording", idle->copyTexture({.texture = texture}, {.texture = texture2}, 4, 4, 1));
    CHECK_VALIDATION(f, "not recording", idle->copyBufferToTexture({.texture = texture}, {b, 0, 256}, 1, 1));
    CHECK_VALIDATION(f, "not recording", idle->copyTextureToBuffer({b, 0, 256}, {.texture = texture}, 1, 1));
    CHECK_VALIDATION(f, "not recording", idle->resolveTexture(pass.target, 0, 0, msaa, 0));
    CHECK_VALIDATION(f, "not recording", idle->bindVertexBuffers(0, {}));
    CHECK_VALIDATION(f, "not recording", idle->bindIndexBuffer(a, 0, 64, IndexFormat::Uint16));

    f.device->destroy(idle);
    CHECK_VALIDATIONS(f, 2, "use of a destroyed CommandBuffer", idle->copyBuffer(a, 0, b, 0, 16));
    CHECK_VALIDATIONS(f, 2, "use of a destroyed CommandBuffer",
                      idle->copyTexture({.texture = texture}, {.texture = texture2}, 4, 4, 1));
    CHECK_VALIDATIONS(f, 2, "use of a destroyed CommandBuffer", idle->resolveTexture(pass.target, 0, 0, msaa, 0));
    CHECK_VALIDATIONS(f, 2, "use of a destroyed CommandBuffer",
                      idle->copyBufferToTexture({.texture = texture}, {b, 0, 256}, 1, 1));
    CHECK_VALIDATIONS(f, 2, "use of a destroyed CommandBuffer",
                      idle->copyTextureToBuffer({b, 0, 256}, {.texture = texture}, 1, 1));
}

TEST_CASE("draws")
{
    ValidationFixture f;
    CommandBuffer* outside = f.recording();
    CHECK_VALIDATIONS(f, 2, "not inside a render pass", outside->draw(3, 1)); // and no pipeline
    OpenPass pass(f);
    CommandBuffer* commandBuffer = pass.commandBuffer;
    CHECK_VALIDATION(f, "no graphics pipeline is bound", commandBuffer->draw(3, 1));
    commandBuffer->bindPipeline(f.graphicsPipeline(PixelFormat::R16G16B16A16Float));
    CHECK_VALIDATION(f, "don't match the render pass", commandBuffer->drawIndexed(3, 1, 0, 0, 0));
    commandBuffer->endRendering();

    Texture* depth = f.texture(TextureUsage::DepthStencil, 64, PixelFormat::D32FloatS8Uint);
    AttachmentView* depthView = f.device->createAttachmentView({.texture = depth}).value();
    const ColorAttachment color = {.view = pass.view};
    const DepthStencilAttachment readOnly = {.view = depthView, .readOnlyDepth = true, .readOnlyStencil = true};
    commandBuffer->beginRendering({.colorAttachments = {&color, 1}, .depthStencilAttachment = &readOnly});
    const PixelFormat colorFormats[] = {PixelFormat::R8G8B8A8Unorm};
    Pipeline* writesDepth = f.device
                                ->createGraphicsPipeline({.vertex = {.binaries = f.binaries},
                                                          .depthStencil = {.depthTest = true, .depthWrite = true},
                                                          .colorFormats = colorFormats,
                                                          .depthStencilFormat = PixelFormat::D32FloatS8Uint})
                                .value();
    commandBuffer->bindPipeline(writesDepth);
    CHECK_VALIDATION(f, "the pipeline writes depth, but the pass's depth is read-only", commandBuffer->draw(3, 1));
    GraphicsPipelineDesc stencilDesc = {.vertex = {.binaries = f.binaries},
                                        .depthStencil = {.stencilTest = true},
                                        .colorFormats = colorFormats,
                                        .depthStencilFormat = PixelFormat::D32FloatS8Uint};
    stencilDesc.depthStencil.front.passOp = StencilOp::Replace;
    Pipeline* writesStencil = f.device->createGraphicsPipeline(stencilDesc).value();
    commandBuffer->bindPipeline(writesStencil);
    CHECK_VALIDATION(f, "the pipeline writes stencil, but the pass's stencil is read-only", commandBuffer->draw(3, 1));
    commandBuffer->endRendering();
    f.device->destroy(depthView);
    f.device->destroy(depth);
}

TEST_CASE("bindings and dynamic state")
{
    ValidationFixture f;
    OpenPass pass(f);
    CommandBuffer* commandBuffer = pass.commandBuffer;
    Buffer* buffer = f.buffer(1024);
    std::vector<VertexBufferBinding> many(limits::maxVertexBufferSlots + 1, {buffer, 0, 16, 16});
    CHECK_VALIDATION(f, "slots beyond limits::maxVertexBufferSlots", commandBuffer->bindVertexBuffers(0, many));
    const VertexBufferBinding pastEnd = {buffer, 1020, 16, 16};
    CHECK_VALIDATION(f, "a binding is null or ends past its buffer",
                     commandBuffer->bindVertexBuffers(0, {&pastEnd, 1}));
    CHECK_VALIDATION(f, "the buffer is null or the range ends past it",
                     commandBuffer->bindIndexBuffer(buffer, 1020, 16, IndexFormat::Uint16));
    CHECK_VALIDATION(f, "isn't a multiple of the index size",
                     commandBuffer->bindIndexBuffer(buffer, 2, 16, IndexFormat::Uint32));
    CHECK_VALIDATION(f, "viewports", commandBuffer->setViewports({}));
    CHECK_VALIDATION(f, "scissors", commandBuffer->setScissors({}));
    CHECK_VALIDATION(f, "bounds must satisfy 0 <= min <= max <= 1", commandBuffer->setDepthBounds(0.5f, 0.25f));
    f.common->capabilities.depthBounds = false;
    CHECK_VALIDATION(f, "the depth bounds test is not supported", commandBuffer->setDepthBounds(0.0f, 1.0f));

    Buffer* doomed = f.buffer(1024);
    f.device->destroy(doomed);
    const VertexBufferBinding destroyed = {doomed, 0, 16, 16};
    CHECK_VALIDATION(f, "use of a destroyed Buffer", commandBuffer->bindVertexBuffers(0, {&destroyed, 1}));
    CHECK_VALIDATION(f, "use of a destroyed Buffer",
                     commandBuffer->bindIndexBuffer(doomed, 0, 16, IndexFormat::Uint16));

    commandBuffer->endRendering();
    (void)commandBuffer->end();
    (void)commandBuffer->reset();
    f.device->destroy(commandBuffer);
    const float blend[4] = {};
    CHECK_VALIDATION(f, "use of a destroyed CommandBuffer", commandBuffer->setBlendConstant(blend));
    CHECK_VALIDATION(f, "use of a destroyed CommandBuffer", commandBuffer->setStencilReference(1));
    CHECK_VALIDATIONS(f, 2, "use of a destroyed CommandBuffer", commandBuffer->setDepthBounds(0.0f, 1.0f));
    CHECK_VALIDATIONS(f, 2, "use of a destroyed CommandBuffer", commandBuffer->setViewports({}));
    CHECK_VALIDATIONS(f, 2, "use of a destroyed CommandBuffer", commandBuffer->setScissors({}));
    CHECK_VALIDATIONS(f, 2, "use of a destroyed CommandBuffer", commandBuffer->bindVertexBuffers(0, {}));
    CHECK_VALIDATIONS(f, 2, "use of a destroyed CommandBuffer",
                      commandBuffer->bindIndexBuffer(buffer, 0, 16, IndexFormat::Uint16));
    CHECK_VALIDATIONS(f, 3, "use of a destroyed CommandBuffer", commandBuffer->draw(3, 1));
    CHECK_VALIDATIONS(f, 3, "use of a destroyed CommandBuffer", commandBuffer->drawIndexed(3, 1, 0, 0, 0));
    CHECK_VALIDATIONS(f, 2, "use of a destroyed CommandBuffer", commandBuffer->endRendering());
    CHECK_VALIDATIONS(f, 3, "use of a destroyed CommandBuffer",
                      commandBuffer->beginRendering({.colorAttachments = {}}));
}

TEST_CASE("indirect draws and dispatches")
{
    ValidationFixture f;
    OpenPass pass(f);
    CommandBuffer* commandBuffer = pass.commandBuffer;
    commandBuffer->bindPipeline(f.graphicsPipeline());
    Buffer* arguments = f.buffer(256);
    CHECK_VALIDATION(f, "too small for 100 commands", commandBuffer->drawIndirect(arguments, 0, 100));
    CHECK_VALIDATION(f, "the count buffer offset is misaligned or past the buffer",
                     commandBuffer->drawIndirectCount(arguments, 0, arguments, 254, 1));
    CHECK_VALIDATION(f, "the count buffer is null", commandBuffer->drawIndirectCount(arguments, 0, nullptr, 0, 1));
    CHECK_VALIDATION(f, "the count buffer is null",
                     commandBuffer->drawIndexedIndirectCount(arguments, 0, nullptr, 0, 1));
    commandBuffer->endRendering();
    CHECK_VALIDATION(f, "no compute pipeline is bound", commandBuffer->dispatchIndirect(arguments, 0));

    Buffer* doomed = f.buffer(256);
    f.device->destroy(doomed);
    commandBuffer->bindPipeline(f.computePipeline());
    CHECK_VALIDATION(f, "use of a destroyed Buffer", commandBuffer->dispatchIndirect(doomed, 0));
    const ColorAttachment color = {.view = pass.view};
    commandBuffer->beginRendering({.colorAttachments = {&color, 1}});
    commandBuffer->bindPipeline(f.graphicsPipeline());
    CHECK_VALIDATION(f, "use of a destroyed Buffer", commandBuffer->drawIndirect(doomed, 0, 1));
    CHECK_VALIDATION(f, "use of a destroyed Buffer", commandBuffer->drawIndexedIndirect(doomed, 0, 1));
    CHECK_VALIDATIONS(f, 2, "use of a destroyed Buffer", commandBuffer->drawIndirectCount(doomed, 0, doomed, 0, 1));
    CHECK_VALIDATIONS(f, 2, "use of a destroyed Buffer",
                      commandBuffer->drawIndexedIndirectCount(doomed, 0, doomed, 0, 1));
    commandBuffer->endRendering();

    (void)commandBuffer->end();
    (void)commandBuffer->reset();
    f.device->destroy(commandBuffer);
    CHECK_VALIDATIONS(f, 3, "use of a destroyed CommandBuffer", commandBuffer->drawIndirect(arguments, 0, 1));
    CHECK_VALIDATIONS(f, 3, "use of a destroyed CommandBuffer", commandBuffer->drawIndexedIndirect(arguments, 0, 1));
    CHECK_VALIDATIONS(f, 3, "use of a destroyed CommandBuffer",
                      commandBuffer->drawIndirectCount(arguments, 0, arguments, 0, 1));
    CHECK_VALIDATIONS(f, 3, "use of a destroyed CommandBuffer",
                      commandBuffer->drawIndexedIndirectCount(arguments, 0, arguments, 0, 1));
    CHECK_VALIDATIONS(f, 3, "use of a destroyed CommandBuffer", commandBuffer->dispatchIndirect(arguments, 0));
}
