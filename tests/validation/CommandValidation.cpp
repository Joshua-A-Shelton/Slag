// Validation of command recording outside render passes: state, constants,
// dispatches, barriers, copies, resolves, pipelines, and submits.

#include "ValidationFixture.h"

using namespace slag;

TEST_CASE("command buffer state")
{
    ValidationFixture f;
    CommandBuffer* commandBuffer = f.device->createCommandBuffer({}).value();
    CHECK_VALIDATION(f, "is not begun, not recording", commandBuffer->pushConstants(f.spirvWords, 4, 0));
    CHECK_VALIDATION(f, "is not begun, not recording", (void)commandBuffer->end());
    REQUIRE(static_cast<bool>(commandBuffer->begin()));
    CHECK_VALIDATION(f, "is recording; reset it first", (void)commandBuffer->begin());
    commandBuffer->beginLabel("open");
    CHECK_VALIDATION(f, "1 labels are still open", (void)commandBuffer->end());
    (void)commandBuffer->reset();
    REQUIRE(static_cast<bool>(commandBuffer->begin()));
    CHECK_VALIDATION(f, "pipeline is null", commandBuffer->bindPipeline(nullptr));
    CHECK_VALIDATION(f, "exceeds limits::maxPushConstantsSize", commandBuffer->pushConstants(f.spirvWords, 8, 124));
    CHECK_VALIDATION(f, "must be multiples of 4", commandBuffer->pushConstants(f.spirvWords, 6, 0));
    CHECK_VALIDATION(f, "the address is not a multiple of", commandBuffer->setConstantBuffer(0x10004, 256));
    CHECK_VALIDATION(f, "is not a multiple of 256 up to", commandBuffer->setConstantBuffer(0x10000, 100));
    CHECK_VALIDATION(f, "no compute pipeline is bound", commandBuffer->dispatch(1, 1, 1));
    CommandBuffer* copy = f.recording(CommandQueueType::Copy);
    CHECK_VALIDATION(f, "copy command buffers have no shader state", copy->setConstantBuffer(0x10000, 256));

    ValidationFixture other;
    CommandBuffer* foreign = other.device->createCommandBuffer({}).value();
    CHECK_VALIDATION(f, "the command buffer belongs to another device", f.device->destroy(foreign));
    other.device->destroy(foreign);

    Pipeline* pipeline = f.computePipeline();
    f.device->destroy(pipeline);
    CHECK_VALIDATION(f, "use of a destroyed Pipeline", commandBuffer->bindPipeline(pipeline));
    CHECK_VALIDATION(f, "use of a destroyed Pipeline", (void)pipeline->userData());
    CHECK_VALIDATION(f, "use of a destroyed Pipeline", pipeline->setUserData(1));
    CHECK_VALIDATION(f, "use of a destroyed Pipeline", f.device->destroy(pipeline));
    Pipeline* foreignPipeline = other.computePipeline();
    CHECK_VALIDATION(f, "the pipeline belongs to another device", f.device->destroy(foreignPipeline));
    other.device->destroy(foreignPipeline);

    f.device->destroy(copy);
    (void)commandBuffer->reset(); // destroyed in the initial state
    f.device->destroy(commandBuffer);
    CHECK_VALIDATIONS(f, 3, "use of a destroyed CommandBuffer", commandBuffer->bindPipeline(nullptr));
    CHECK_VALIDATIONS(f, 2, "use of a destroyed CommandBuffer", commandBuffer->pushConstants(f.spirvWords, 4, 0));
    CHECK_VALIDATIONS(f, 2, "use of a destroyed CommandBuffer", commandBuffer->setConstantBuffer(0x10000, 256));
    CHECK_VALIDATIONS(f, 3, "use of a destroyed CommandBuffer", commandBuffer->dispatch(1, 1, 1));
    CHECK_VALIDATIONS(f, 2, "use of a destroyed CommandBuffer", (void)commandBuffer->end());
    CHECK_VALIDATION(f, "use of a destroyed CommandBuffer", (void)commandBuffer->reset());
    CHECK_VALIDATION(f, "use of a destroyed CommandBuffer", (void)commandBuffer->userData());
    CHECK_VALIDATION(f, "use of a destroyed CommandBuffer", commandBuffer->setUserData(1));
    CHECK_VALIDATION(f, "use of a destroyed CommandBuffer", (void)commandBuffer->begin());
    CHECK_VALIDATION(f, "use of a destroyed CommandBuffer", f.device->destroy(commandBuffer));
}

TEST_CASE("barriers")
{
    ValidationFixture f;
    CommandBuffer* commandBuffer = f.recording();
    BufferBarrier noBuffer;
    CHECK_VALIDATION(f, "a buffer barrier has no buffer", commandBuffer->barrier({}, {&noBuffer, 1}));
    TextureBarrier noTexture;
    CHECK_VALIDATION(f, "a texture barrier has no texture", commandBuffer->barrier({}, {}, {&noTexture, 1}));
    Texture* texture = f.texture();
    TextureBarrier mips = {.texture = texture, .range = {.baseMip = 1}};
    CHECK_VALIDATION(f, "the mip range is outside the texture", commandBuffer->barrier({}, {}, {&mips, 1}));
    TextureBarrier layers = {.texture = texture, .range = {.baseLayer = 1}};
    CHECK_VALIDATION(f, "the layer range is outside the texture", commandBuffer->barrier({}, {}, {&layers, 1}));
    TextureBarrier plane = {.texture = texture, .range = {.plane = TexturePlane::Stencil}};
    CHECK_VALIDATION(f, "the texture has no such plane", commandBuffer->barrier({}, {}, {&plane, 1}));
    CommandBuffer* copy = f.recording(CommandQueueType::Copy);
    TextureBarrier transition = {.layoutAfter = TextureLayout::CopyDest, .texture = texture};
    CHECK_VALIDATION(f, "textures stay in the Common layout on copy queues", copy->barrier({}, {}, {&transition, 1}));

    Buffer* buffer = f.buffer();
    f.device->destroy(buffer);
    BufferBarrier destroyedBuffer = {.buffer = buffer};
    CHECK_VALIDATION(f, "use of a destroyed Buffer", commandBuffer->barrier({}, {&destroyedBuffer, 1}));
    f.device->destroy(texture);
    TextureBarrier destroyedTexture = {.texture = texture};
    CHECK_VALIDATION(f, "use of a destroyed Texture", commandBuffer->barrier({}, {}, {&destroyedTexture, 1}));
}

TEST_CASE("buffer copies")
{
    ValidationFixture f;
    CommandBuffer* commandBuffer = f.recording();
    Buffer* a = f.buffer(1024);
    Buffer* b = f.buffer(1024);
    CHECK_VALIDATION(f, "a buffer is null", commandBuffer->copyBuffer(nullptr, 0, b, 0, 16));
    CHECK_VALIDATION(f, "the destination range ends past the buffer", commandBuffer->copyBuffer(a, 1020, b, 0, 16));
    CHECK_VALIDATION(f, "the source range ends past the buffer", commandBuffer->copyBuffer(a, 0, b, 1020, 16));
    f.device->destroy(a);
    CHECK_VALIDATION(f, "use of a destroyed Buffer", commandBuffer->copyBuffer(a, 0, b, 0, 16));
    CHECK_VALIDATION(f, "use of a destroyed Buffer", commandBuffer->copyBuffer(b, 0, a, 0, 16));
}

TEST_CASE("texture copies")
{
    ValidationFixture f;
    CommandBuffer* commandBuffer = f.recording();
    Texture* a = f.texture(TextureUsage::None, 64, PixelFormat::R8G8B8A8Unorm, 1, 2);
    Texture* b = f.texture(TextureUsage::None, 64);
    CHECK_VALIDATION(f, "a texture is null", commandBuffer->copyTexture({.texture = a}, {}, 4, 4, 1));
    Texture* msaa = f.texture(TextureUsage::RenderTarget, 64, PixelFormat::R8G8B8A8Unorm, 4);
    CHECK_VALIDATION(f, "MSAA textures can't be copied",
                     commandBuffer->copyTexture({.texture = a}, {.texture = msaa}, 4, 4, 1));
    CHECK_VALIDATION(f, "mip 2 is outside the texture",
                     commandBuffer->copyTexture({.texture = a, .mip = 2}, {.texture = b}, 1, 1, 1));
    CHECK_VALIDATION(f, "layer 1 is outside the texture",
                     commandBuffer->copyTexture({.texture = a, .layer = 1}, {.texture = b}, 4, 4, 1));
    CHECK_VALIDATION(f, "is outside the mip",
                     commandBuffer->copyTexture({.texture = a, .x = 62}, {.texture = b}, 4, 4, 1));
    Texture* bc = f.texture(TextureUsage::None, 64, PixelFormat::BC1Unorm);
    Texture* bc2 = f.texture(TextureUsage::None, 64, PixelFormat::BC1Unorm);
    CHECK_VALIDATION(f, "cover whole 4x4 blocks",
                     commandBuffer->copyTexture({.texture = bc, .x = 2}, {.texture = bc2, .x = 4}, 4, 4, 1));
    Texture* depth = f.texture(TextureUsage::DepthStencil, 64, PixelFormat::D32Float);
    Texture* depth2 = f.texture(TextureUsage::DepthStencil, 64, PixelFormat::D32Float);
    CHECK_VALIDATION(f, "the plane doesn't fit the format",
                     commandBuffer->copyTexture({.texture = depth, .plane = TexturePlane::Stencil},
                                                {.texture = depth2, .plane = TexturePlane::Depth}, 4, 4, 1));
    Texture* floats = f.texture(TextureUsage::None, 64, PixelFormat::R32G32B32A32Float);
    CHECK_VALIDATION(f, "are neither the same nor castable",
                     commandBuffer->copyTexture({.texture = a}, {.texture = floats}, 4, 4, 1));

    Buffer* buffer = f.buffer(1 << 20);
    CHECK_VALIDATION(f, "the buffer is null",
                     commandBuffer->copyBufferToTexture({.texture = b}, {nullptr, 0, 256}, 4, 4));
    CHECK_VALIDATION(f, "the buffer offset 4 is not a multiple",
                     commandBuffer->copyBufferToTexture({.texture = b}, {buffer, 4, 256}, 4, 4));
    CHECK_VALIDATION(f, "the row pitch 100 is not a multiple",
                     commandBuffer->copyTextureToBuffer({buffer, 0, 100}, {.texture = b}, 4, 4));
    CHECK_VALIDATION(f, "is smaller than a row",
                     commandBuffer->copyBufferToTexture({.texture = b}, {buffer, 0, 0}, 4, 4));
    Buffer* small = f.buffer(512);
    CHECK_VALIDATION(f, "the region ends past the buffer",
                     commandBuffer->copyTextureToBuffer({small, 0, 256}, {.texture = b}, 4, 4));

    f.device->destroy(b);
    f.device->destroy(buffer);
    CHECK_VALIDATION(f, "use of a destroyed Texture",
                     commandBuffer->copyTexture({.texture = a}, {.texture = b}, 4, 4, 1));
    CHECK_VALIDATION(f, "use of a destroyed Texture",
                     commandBuffer->copyTexture({.texture = b}, {.texture = a}, 4, 4, 1));
    Buffer* alive = f.buffer(1 << 20);
    CHECK_VALIDATION(f, "use of a destroyed Texture",
                     commandBuffer->copyBufferToTexture({.texture = b}, {alive, 0, 256}, 4, 4));
    CHECK_VALIDATION(f, "use of a destroyed Buffer",
                     commandBuffer->copyBufferToTexture({.texture = a}, {buffer, 0, 256}, 4, 4));
    CHECK_VALIDATION(f, "use of a destroyed Buffer",
                     commandBuffer->copyTextureToBuffer({buffer, 0, 256}, {.texture = a}, 4, 4));
    CHECK_VALIDATION(f, "use of a destroyed Texture",
                     commandBuffer->copyTextureToBuffer({alive, 0, 256}, {.texture = b}, 4, 4));
}

TEST_CASE("resolves")
{
    ValidationFixture f;
    CommandBuffer* commandBuffer = f.recording();
    Texture* msaa = f.texture(TextureUsage::RenderTarget, 64, PixelFormat::R8G8B8A8Unorm, 4);
    Texture* single = f.texture(TextureUsage::RenderTarget, 64);
    CommandBuffer* compute = f.recording(CommandQueueType::Compute);
    CHECK_VALIDATION(f, "resolves need a graphics queue", compute->resolveTexture(single, 0, 0, msaa, 0));
    CHECK_VALIDATION(f, "a texture is null", commandBuffer->resolveTexture(nullptr, 0, 0, msaa, 0));
    CHECK_VALIDATION(f, "from an MSAA texture into a single-sampled one",
                     commandBuffer->resolveTexture(msaa, 0, 0, single, 0));
    Texture* floats = f.texture(TextureUsage::RenderTarget, 64, PixelFormat::R16G16B16A16Float);
    CHECK_VALIDATION(f, "the formats differ", commandBuffer->resolveTexture(floats, 0, 0, msaa, 0));
    Texture* uintMsaa = f.texture(TextureUsage::RenderTarget, 64, PixelFormat::R8G8B8A8Uint, 4);
    Texture* uintSingle = f.texture(TextureUsage::RenderTarget, 64, PixelFormat::R8G8B8A8Uint);
    CHECK_VALIDATION(f, "integer and depth formats can't be resolved",
                     commandBuffer->resolveTexture(uintSingle, 0, 0, uintMsaa, 0));
    CHECK_VALIDATION(f, "the mip or a layer is outside its texture",
                     commandBuffer->resolveTexture(single, 0, 1, msaa, 0));
    Texture* bigger = f.texture(TextureUsage::RenderTarget, 128);
    CHECK_VALIDATION(f, "differ in size", commandBuffer->resolveTexture(bigger, 0, 0, msaa, 0));
    f.device->destroy(single);
    CHECK_VALIDATION(f, "use of a destroyed Texture", commandBuffer->resolveTexture(single, 0, 0, msaa, 0));
    Texture* destination = f.texture(TextureUsage::RenderTarget, 64);
    f.device->destroy(msaa);
    CHECK_VALIDATION(f, "use of a destroyed Texture", commandBuffer->resolveTexture(destination, 0, 0, msaa, 0));
}

TEST_CASE("graphics pipelines")
{
    ValidationFixture f;
    auto create = [&](GraphicsPipelineDesc desc) {
        if (desc.vertex.binaries.empty() && desc.debugName == nullptr)
        {
            desc.vertex = {.binaries = f.binaries};
        }
        f.device->destroy(f.device->createGraphicsPipeline(desc).value());
    };
    const PixelFormat rgba[] = {PixelFormat::R8G8B8A8Unorm};
    CHECK_VALIDATION(f, "no vertex shader binaries", create({.colorFormats = rgba, .debugName = "no shaders"}));
    PixelFormat nine[9];
    for (PixelFormat& format : nine)
    {
        format = PixelFormat::R8G8B8A8Unorm;
    }
    CHECK_VALIDATION(f, "9 color targets exceed", create({.colorFormats = nine}));
    CHECK_VALIDATION(f, "sample count 3 is not 1, 2, 4, or 8", create({.colorFormats = rgba, .sampleCount = 3}));
    auto without = [&](PixelFormat format, FormatSupport removed) {
        f.common->formatSupport[static_cast<size_t>(format)] &= ~removed;
    };
    without(PixelFormat::R8G8B8A8Snorm, FormatSupport::RenderTarget);
    const PixelFormat snorm[] = {PixelFormat::R8G8B8A8Snorm};
    CHECK_VALIDATION(f, "can't be a render target", create({.colorFormats = snorm}));
    without(PixelFormat::R32G32B32A32Uint, FormatSupport::Blend | FormatSupport::Multisample4);
    const PixelFormat uints[] = {PixelFormat::R32G32B32A32Uint};
    GraphicsPipelineDesc blended = {.colorFormats = uints};
    blended.blend.targets[0].blendEnable = true;
    CHECK_VALIDATION(f, "doesn't support blending", create(blended));
    CHECK_VALIDATION(f, "doesn't support 4x MSAA", create({.colorFormats = uints, .sampleCount = 4}));
    const PixelFormat two[] = {PixelFormat::R8G8B8A8Unorm, PixelFormat::R8G8B8A8Unorm};
    GraphicsPipelineDesc dual = {.colorFormats = two};
    dual.blend.targets[1] = {.blendEnable = true, .srcColorFactor = BlendFactor::Src1Color};
    CHECK_VALIDATION(f, "dual-source blending is for render target 0 only", create(dual));
    GraphicsPipelineDesc colorAlpha = {.colorFormats = rgba};
    colorAlpha.blend.targets[0] = {.blendEnable = true, .srcAlphaFactor = BlendFactor::SrcColor};
    CHECK_VALIDATION(f, "alpha blend factors can't be color factors", create(colorAlpha));
    without(PixelFormat::D16Unorm, FormatSupport::DepthStencil);
    CHECK_VALIDATION(f, "can't be a depth/stencil target", create({.depthStencilFormat = PixelFormat::D16Unorm}));
    without(PixelFormat::D32Float, FormatSupport::Multisample8);
    CHECK_VALIDATION(f, "doesn't support 8x MSAA",
                     create({.depthStencilFormat = PixelFormat::D32Float, .sampleCount = 8}));
    CHECK_VALIDATION(f, "depth or stencil testing without a depth/stencil format",
                     create({.depthStencil = {.depthTest = true}}));
    CHECK_VALIDATION(f, "stencil testing needs a format with stencil",
                     create({.depthStencil = {.stencilTest = true}, .depthStencilFormat = PixelFormat::D32Float}));
    f.common->capabilities.depthBounds = false;
    CHECK_VALIDATION(f, "the depth bounds test is not supported",
                     create({.depthStencil = {.depthTest = true, .depthBoundsTest = true},
                             .depthStencilFormat = PixelFormat::D32Float}));
    f.common->capabilities.conservativeRasterization = false;
    CHECK_VALIDATION(f, "conservative rasterization is not supported",
                     create({.rasterizer = {.conservativeRasterization = true}, .colorFormats = rgba}));

    VertexAttribute attributes[limits::maxVertexAttributes + 1] = {};
    for (uint32_t i = 0; i <= limits::maxVertexAttributes; ++i)
    {
        attributes[i] = {.location = i, .format = PixelFormat::R32Float, .slot = 0};
    }
    const VertexStepRate slot[] = {VertexStepRate::PerVertex};
    CHECK_VALIDATION(f, "vertex attributes exceed",
                     create({.vertexInput = {.attributes = attributes, .slots = slot}, .colorFormats = rgba}));
    VertexStepRate slots[limits::maxVertexBufferSlots + 1] = {};
    CHECK_VALIDATION(f, "vertex buffer slots exceed",
                     create({.vertexInput = {.attributes = {attributes, 1}, .slots = slots}, .colorFormats = rgba}));
    const VertexAttribute unslotted[] = {{.location = 0, .format = PixelFormat::R32Float, .slot = 1}};
    CHECK_VALIDATION(f, "only 1 slots are declared",
                     create({.vertexInput = {.attributes = unslotted, .slots = slot}, .colorFormats = rgba}));
    without(PixelFormat::BC1Unorm, FormatSupport::VertexBuffer);
    const VertexAttribute notVertex[] = {{.location = 0, .format = PixelFormat::BC1Unorm, .slot = 0}};
    CHECK_VALIDATION(f, "is not a vertex format",
                     create({.vertexInput = {.attributes = notVertex, .slots = slot}, .colorFormats = rgba}));
    const VertexAttribute twice[] = {{.location = 2, .format = PixelFormat::R32Float, .slot = 0},
                                     {.location = 2, .format = PixelFormat::R32Float, .slot = 0}};
    CHECK_VALIDATION(f, "two attributes use location 2",
                     create({.vertexInput = {.attributes = twice, .slots = slot}, .colorFormats = rgba}));
    CHECK_VALIDATION(f, "no shader binaries", (void)f.device->createComputePipeline({}));

    ValidationFixture uninitialized(false);
    CHECK_VALIDATION(uninitialized, "is not initialized",
                     (void)uninitialized.device->createGraphicsPipeline({.vertex = {.binaries = f.binaries}}));
    CHECK_VALIDATION(uninitialized, "is not initialized",
                     (void)uninitialized.device->createComputePipeline({.compute = {.binaries = f.binaries}}));
}

TEST_CASE("submits")
{
    ValidationFixture f;
    CommandQueue* graphics = f.device->commandQueue(CommandQueueType::Graphics);
    CommandBuffer* unfinished = f.recording();
    CommandBuffer* list[] = {unfinished};
    CHECK_VALIDATION(f, "is recording, not ended", (void)graphics->submit({.commandBuffers = list}));
    CommandBuffer* compute = f.recording(CommandQueueType::Compute);
    (void)compute->end();
    CommandBuffer* computeList[] = {compute};
    CHECK_VALIDATION(f, "was created for a different queue type",
                     (void)graphics->submit({.commandBuffers = computeList}));

    Texture* target = f.texture(TextureUsage::RenderTarget);
    AttachmentView* view = f.device->createAttachmentView({.texture = target}).value();
    const ColorAttachment color = {.view = view};
    CommandBuffer* suspends = f.recording();
    suspends->beginRendering({.colorAttachments = {&color, 1}, .suspending = true});
    suspends->endRendering();
    (void)suspends->end();
    CommandBuffer* resumes = f.recording();
    resumes->beginRendering({.colorAttachments = {&color, 1}, .resuming = true});
    resumes->endRendering();
    (void)resumes->end();
    CommandBuffer* alone[] = {suspends};
    CHECK_VALIDATION(f, "suspends a render pass the next one doesn't resume",
                     (void)graphics->submit({.commandBuffers = alone}));
    CommandBuffer* orphan[] = {resumes};
    CHECK_VALIDATION(f, "resumes a render pass the previous one didn't suspend",
                     (void)graphics->submit({.commandBuffers = orphan}));

    Semaphore* semaphore = f.device->createSemaphore({}).value();
    f.device->destroy(semaphore);
    SemaphoreValue value = {semaphore, 1};
    CHECK_VALIDATION(f, "use of a destroyed Semaphore", (void)graphics->submit({.waits = {&value, 1}}));
    CHECK_VALIDATION(f, "use of a destroyed Semaphore", (void)graphics->submit({.signals = {&value, 1}}));
    CommandBuffer* doomed = f.recording();
    (void)doomed->end();
    f.device->destroy(doomed);
    CommandBuffer* destroyed[] = {doomed};
    CHECK_VALIDATION(f, "use of a destroyed CommandBuffer", (void)graphics->submit({.commandBuffers = destroyed}));
    f.device->destroy(view);
    f.device->destroy(target);
}
