#include "TestDevice.h"

#include <slag/AttachmentView.h>
#include <slag/Buffer.h>
#include <slag/CommandBuffer.h>
#include <slag/GraphicsDevice.h>
#include <slag/Pipeline.h>
#include <slag/Semaphore.h>
#include <slag/Texture.h>

#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace {

std::vector<char> readFile(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

// Both formats of one shader entry point, kept alive for pipeline creation.
struct ShaderFiles
{
    std::vector<char> spirv;
    std::vector<char> dxil;
    std::vector<slag::ShaderBinary> binaries;

    ShaderFiles(const std::string& name, const std::string& entry)
    {
        spirv = readFile(std::string(SLAG_SHADER_DIR "/") + name + "." + entry + ".spv");
        dxil = readFile(std::string(SLAG_SHADER_DIR "/") + name + "." + entry + ".dxil");
        if (!dxil.empty())
        {
            binaries.push_back({slag::ShaderFormat::Dxil, dxil.data(), dxil.size()});
        }
        if (!spirv.empty())
        {
            binaries.push_back({slag::ShaderFormat::Spirv, spirv.data(), spirv.size()});
        }
    }
};

struct Vertex
{
    float x, y;
    float r, g, b, a;
};

// One triangle in the top-left quadrant, clip-space +Y up: checks coverage,
// the clear, and that both backends put +Y at the top of the image.
void testTriangle(slag::Backend backend)
{
    TestInstance test(backend);
    slag::GraphicsDevice* device = test.firstDevice();
    REQUIRE(device);
    REQUIRE(static_cast<bool>(device->initialize({})));

    ShaderFiles vertexShader("Triangle", "vertexMain");
    ShaderFiles pixelShader("Triangle", "pixelMain");
    const slag::VertexAttribute attributes[] = {
        {.location = 0, .format = slag::PixelFormat::R32G32Float, .slot = 0, .offset = 0},
        {.location = 1, .format = slag::PixelFormat::R32G32B32A32Float, .slot = 0, .offset = 8}};
    const slag::VertexStepRate slots[] = {slag::VertexStepRate::PerVertex};
    const slag::PixelFormat colorFormats[] = {slag::PixelFormat::R8G8B8A8Unorm};
    slag::Result<slag::Pipeline*> pipeline =
        device->createGraphicsPipeline({.vertex = {.binaries = vertexShader.binaries, .entryPoint = "vertexMain"},
                                        .pixel = {.binaries = pixelShader.binaries, .entryPoint = "pixelMain"},
                                        .vertexInput = {.attributes = attributes, .slots = slots},
                                        .colorFormats = colorFormats,
                                        .debugName = "triangle"});
    REQUIRE(static_cast<bool>(pipeline));

    constexpr uint32_t size = 64;
    slag::Texture* target = device
                                ->createTexture2D({.format = slag::PixelFormat::R8G8B8A8Unorm,
                                                   .width = size,
                                                   .height = size,
                                                   .usage = slag::TextureUsage::RenderTarget})
                                .value();
    REQUIRE(target);
    slag::AttachmentView* view = device->createAttachmentView({.texture = target}).value();
    REQUIRE(view);

    const Vertex vertices[] = {{-1.0f, 1.0f, 1, 0, 0, 1}, {0.0f, 1.0f, 1, 0, 0, 1}, {-1.0f, 0.0f, 1, 0, 0, 1}};
    slag::Buffer* vertexBuffer = device
                                     ->createBuffer({.size = sizeof(vertices),
                                                     .placement = slag::Placement::automatic(slag::MemoryKind::Upload)})
                                     .value();
    REQUIRE(vertexBuffer);
    std::memcpy(vertexBuffer->mappedData(), vertices, sizeof(vertices));
    slag::CopyFootprint footprint = device->copyFootprint(target, 0);
    slag::Buffer* readback = device
                                 ->createBuffer({.size = footprint.size,
                                                 .placement = slag::Placement::automatic(slag::MemoryKind::Readback)})
                                 .value();
    REQUIRE(readback);

    slag::CommandBuffer* commandBuffer =
        device->createCommandBuffer({.queueType = slag::CommandQueueType::Graphics}).value();
    REQUIRE(commandBuffer);
    REQUIRE(static_cast<bool>(commandBuffer->begin()));
    slag::TextureBarrier toTarget = {.syncBefore = slag::BarrierSync::None,
                                     .syncAfter = slag::BarrierSync::RenderTarget,
                                     .accessAfter = slag::BarrierAccess::RenderTarget,
                                     .layoutBefore = slag::TextureLayout::Undefined,
                                     .layoutAfter = slag::TextureLayout::RenderTarget,
                                     .texture = target};
    commandBuffer->barrier({}, {}, {&toTarget, 1});
    const slag::ColorAttachment color = {.view = view,
                                         .loadOp = slag::LoadOp::Clear,
                                         .storeOp = slag::StoreOp::Store,
                                         .clearValue = {.color = {0.0f, 0.0f, 1.0f, 1.0f}}};
    commandBuffer->beginRendering({.colorAttachments = {&color, 1}});
    commandBuffer->bindPipeline(pipeline.value());
    const slag::VertexBufferBinding binding = {vertexBuffer, 0, sizeof(vertices), sizeof(Vertex)};
    commandBuffer->bindVertexBuffers(0, {&binding, 1});
    commandBuffer->draw(3, 1);
    commandBuffer->endRendering();
    slag::TextureBarrier toCopy = {.syncBefore = slag::BarrierSync::RenderTarget,
                                   .syncAfter = slag::BarrierSync::Copy,
                                   .accessBefore = slag::BarrierAccess::RenderTarget,
                                   .accessAfter = slag::BarrierAccess::CopySource,
                                   .layoutBefore = slag::TextureLayout::RenderTarget,
                                   .layoutAfter = slag::TextureLayout::CopySource,
                                   .texture = target};
    commandBuffer->barrier({}, {}, {&toCopy, 1});
    commandBuffer->copyTextureToBuffer({readback, 0, footprint.rowPitch}, {.texture = target}, size, size);
    REQUIRE(static_cast<bool>(commandBuffer->end()));

    slag::Semaphore* semaphore = device->createSemaphore({}).value();
    slag::SemaphoreValue done = {semaphore, 1};
    slag::CommandBuffer* commandBuffers[] = {commandBuffer};
    REQUIRE(static_cast<bool>(device->commandQueue(slag::CommandQueueType::Graphics)
                                  ->submit({.commandBuffers = commandBuffers, .signals = {&done, 1}})));
    REQUIRE(static_cast<bool>(device->waitSemaphores({&done, 1}, slag::WaitMode::All, 5'000'000'000)));

    auto pixel = [&](uint32_t x, uint32_t y) {
        const uint8_t* p = static_cast<const uint8_t*>(readback->mappedData()) + y * footprint.rowPitch + x * 4;
        return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
    };
    constexpr uint32_t red = 0xFF0000FF;
    constexpr uint32_t blue = 0xFFFF0000;
    CHECK(pixel(8, 8) == red);    // inside, top-left
    CHECK(pixel(56, 56) == blue); // bottom-right: cleared
    CHECK(pixel(8, 56) == blue);  // bottom-left: the triangle is at the top
    CHECK(pixel(56, 8) == blue);  // top-right

    device->destroy(semaphore);
    device->destroy(commandBuffer);
    device->destroy(readback);
    device->destroy(vertexBuffer);
    device->destroy(view);
    device->destroy(target);
    device->destroy(pipeline.value());
    device->shutdown();
    test.checkNoErrors();
}

// Submits one command buffer and waits for it.
void submitAndWait(slag::GraphicsDevice* device, slag::CommandBuffer* commandBuffer)
{
    slag::Semaphore* semaphore = device->createSemaphore({}).value();
    REQUIRE(semaphore);
    slag::SemaphoreValue done = {semaphore, 1};
    slag::CommandBuffer* commandBuffers[] = {commandBuffer};
    REQUIRE(static_cast<bool>(device->commandQueue(slag::CommandQueueType::Graphics)
                                  ->submit({.commandBuffers = commandBuffers, .signals = {&done, 1}})));
    REQUIRE(static_cast<bool>(device->waitSemaphores({&done, 1}, slag::WaitMode::All, 5'000'000'000)));
    device->destroy(semaphore);
}

// drawIndirectCount: four argument records, a GPU count of 3, two instances
// each. Checks draw IDs (firstInstance -> SV_StartInstanceLocation), that
// SV_InstanceID starts at 0, and that the count limits the draws.
void testDrawIds(slag::Backend backend)
{
    TestInstance test(backend);
    slag::GraphicsDevice* device = test.firstDevice();
    REQUIRE(device);
    REQUIRE(static_cast<bool>(device->initialize({})));

    ShaderFiles vertexShader("DrawId", "vertexMain");
    ShaderFiles pixelShader("DrawId", "pixelMain");
    const slag::PixelFormat colorFormats[] = {slag::PixelFormat::R8G8B8A8Unorm};
    slag::Pipeline* pipeline =
        device
            ->createGraphicsPipeline({.vertex = {.binaries = vertexShader.binaries, .entryPoint = "vertexMain"},
                                      .pixel = {.binaries = pixelShader.binaries, .entryPoint = "pixelMain"},
                                      .colorFormats = colorFormats})
            .value();
    REQUIRE(pipeline);

    constexpr uint32_t width = 64;
    constexpr uint32_t height = 32;
    slag::Texture* target = device
                                ->createTexture2D({.format = slag::PixelFormat::R8G8B8A8Unorm,
                                                   .width = width,
                                                   .height = height,
                                                   .usage = slag::TextureUsage::RenderTarget})
                                .value();
    slag::AttachmentView* view = device->createAttachmentView({.texture = target}).value();
    REQUIRE(view);

    // Arguments at 0, the count at 256.
    slag::Buffer* arguments =
        device->createBuffer({.size = 512, .placement = slag::Placement::automatic(slag::MemoryKind::Upload)}).value();
    REQUIRE(arguments);
    auto* draws = static_cast<slag::DrawArguments*>(arguments->mappedData());
    for (uint32_t i = 0; i < 4; ++i)
    {
        draws[i] = {.vertexCount = 6, .instanceCount = 2, .firstVertex = 0, .firstInstance = i};
    }
    *reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(arguments->mappedData()) + 256) = 3;
    slag::CopyFootprint footprint = device->copyFootprint(target, 0);
    slag::Buffer* readback = device
                                 ->createBuffer({.size = footprint.size,
                                                 .placement = slag::Placement::automatic(slag::MemoryKind::Readback)})
                                 .value();
    REQUIRE(readback);

    slag::CommandBuffer* commandBuffer =
        device->createCommandBuffer({.queueType = slag::CommandQueueType::Graphics}).value();
    REQUIRE(static_cast<bool>(commandBuffer->begin()));
    slag::TextureBarrier toTarget = {.syncBefore = slag::BarrierSync::None,
                                     .syncAfter = slag::BarrierSync::RenderTarget,
                                     .accessAfter = slag::BarrierAccess::RenderTarget,
                                     .layoutBefore = slag::TextureLayout::Undefined,
                                     .layoutAfter = slag::TextureLayout::RenderTarget,
                                     .texture = target};
    slag::BufferBarrier toIndirect = {.syncBefore = slag::BarrierSync::None,
                                      .syncAfter = slag::BarrierSync::ExecuteIndirect,
                                      .accessAfter = slag::BarrierAccess::IndirectArgument,
                                      .buffer = arguments};
    commandBuffer->barrier({}, {&toIndirect, 1}, {&toTarget, 1});
    const slag::ColorAttachment color = {.view = view, .loadOp = slag::LoadOp::Clear};
    commandBuffer->beginRendering({.colorAttachments = {&color, 1}});
    commandBuffer->bindPipeline(pipeline);
    commandBuffer->drawIndirectCount(arguments, 0, arguments, 256, 4);
    commandBuffer->endRendering();
    slag::TextureBarrier toCopy = {.syncBefore = slag::BarrierSync::RenderTarget,
                                   .syncAfter = slag::BarrierSync::Copy,
                                   .accessBefore = slag::BarrierAccess::RenderTarget,
                                   .accessAfter = slag::BarrierAccess::CopySource,
                                   .layoutBefore = slag::TextureLayout::RenderTarget,
                                   .layoutAfter = slag::TextureLayout::CopySource,
                                   .texture = target};
    commandBuffer->barrier({}, {}, {&toCopy, 1});
    commandBuffer->copyTextureToBuffer({readback, 0, footprint.rowPitch}, {.texture = target}, width, height);
    REQUIRE(static_cast<bool>(commandBuffer->end()));
    submitAndWait(device, commandBuffer);

    // Cell centers: draws 0-2 drawn with their ID and instance; draw 3 not drawn.
    const auto* pixels = static_cast<const uint8_t*>(readback->mappedData());
    for (uint32_t draw = 0; draw < 4; ++draw)
    {
        for (uint32_t instance = 0; instance < 2; ++instance)
        {
            CAPTURE(draw);
            CAPTURE(instance);
            const uint8_t* p = pixels + (instance * 16 + 8) * footprint.rowPitch + (draw * 16 + 8) * 4;
            if (draw < 3)
            {
                CHECK(p[0] == (draw + 1) * 32);
                CHECK(p[1] == (instance + 1) * 64);
                CHECK(p[3] == 255);
            }
            else
            {
                CHECK(p[0] == 0);
                CHECK(p[3] == 0);
            }
        }
    }

    device->destroy(commandBuffer);
    device->destroy(readback);
    device->destroy(arguments);
    device->destroy(view);
    device->destroy(target);
    device->destroy(pipeline);
    device->shutdown();
    test.checkNoErrors();
}

slag::TextureBarrier transition(slag::Texture* texture, slag::TextureLayout before, slag::TextureLayout after)
{
    auto use = [](slag::TextureLayout layout, slag::BarrierSync& sync, slag::BarrierAccess& access) {
        switch (layout)
        {
        case slag::TextureLayout::RenderTarget:
            sync = slag::BarrierSync::RenderTarget;
            access = slag::BarrierAccess::RenderTarget;
            break;
        case slag::TextureLayout::ResolveSource:
            sync = slag::BarrierSync::Resolve;
            access = slag::BarrierAccess::ResolveSource;
            break;
        case slag::TextureLayout::ResolveDest:
            sync = slag::BarrierSync::Resolve;
            access = slag::BarrierAccess::ResolveDest;
            break;
        case slag::TextureLayout::CopySource:
            sync = slag::BarrierSync::Copy;
            access = slag::BarrierAccess::CopySource;
            break;
        default:
            sync = slag::BarrierSync::None;
            access = slag::BarrierAccess::NoAccess;
            break;
        }
    };
    slag::TextureBarrier barrier = {.layoutBefore = before, .layoutAfter = after, .texture = texture};
    use(before, barrier.syncBefore, barrier.accessBefore);
    use(after, barrier.syncAfter, barrier.accessAfter);
    return barrier;
}

// Shared setup for the triangle-based tests.
struct TriangleScene
{
    slag::GraphicsDevice* device = nullptr;
    ShaderFiles vertexShader{"Triangle", "vertexMain"};
    ShaderFiles pixelShader{"Triangle", "pixelMain"};
    slag::Buffer* vertices = nullptr;

    // Triangle 0 red in the top-left quadrant, triangle 1 green in the bottom-right.
    explicit TriangleScene(slag::GraphicsDevice* device)
        : device(device)
    {
        const Vertex data[] = {{-1.0f, 1.0f, 1, 0, 0, 1}, {0.0f, 1.0f, 1, 0, 0, 1},  {-1.0f, 0.0f, 1, 0, 0, 1},
                               {0.0f, -1.0f, 0, 1, 0, 1}, {1.0f, -1.0f, 0, 1, 0, 1}, {1.0f, 0.0f, 0, 1, 0, 1}};
        vertices = device
                       ->createBuffer(
                           {.size = sizeof(data), .placement = slag::Placement::automatic(slag::MemoryKind::Upload)})
                       .value();
        REQUIRE(vertices);
        std::memcpy(vertices->mappedData(), data, sizeof(data));
    }

    ~TriangleScene()
    {
        device->destroy(vertices);
    }

    slag::Pipeline* pipeline(uint32_t sampleCount)
    {
        const slag::VertexAttribute attributes[] = {
            {.location = 0, .format = slag::PixelFormat::R32G32Float, .slot = 0, .offset = 0},
            {.location = 1, .format = slag::PixelFormat::R32G32B32A32Float, .slot = 0, .offset = 8}};
        const slag::VertexStepRate slots[] = {slag::VertexStepRate::PerVertex};
        const slag::PixelFormat colorFormats[] = {slag::PixelFormat::R8G8B8A8Unorm};
        return device
            ->createGraphicsPipeline({.vertex = {.binaries = vertexShader.binaries, .entryPoint = "vertexMain"},
                                      .pixel = {.binaries = pixelShader.binaries, .entryPoint = "pixelMain"},
                                      .vertexInput = {.attributes = attributes, .slots = slots},
                                      .colorFormats = colorFormats,
                                      .sampleCount = sampleCount})
            .value();
    }

    void draw(slag::CommandBuffer* commandBuffer, slag::Pipeline* pipeline, uint32_t triangle)
    {
        commandBuffer->bindPipeline(pipeline);
        const slag::VertexBufferBinding binding = {vertices, 0, 6 * sizeof(Vertex), sizeof(Vertex)};
        commandBuffer->bindVertexBuffers(0, {&binding, 1});
        commandBuffer->draw(3, 1, triangle * 3);
    }
};

slag::Texture* renderTarget(slag::GraphicsDevice* device, uint32_t size, uint32_t sampleCount)
{
    slag::Texture* texture = device
                                 ->createTexture2D({.format = slag::PixelFormat::R8G8B8A8Unorm,
                                                    .width = size,
                                                    .height = size,
                                                    .sampleCount = sampleCount,
                                                    .usage = slag::TextureUsage::RenderTarget})
                                 .value();
    REQUIRE(texture);
    return texture;
}

// 4x MSAA resolved with resolveTexture: averaged pixels along the triangle's
// edge. (Render-pass resolves were cut in M7: D3D12 and Vulkan need the
// destination in different layouts, ADR 0049.)
void testResolves(slag::Backend backend)
{
    TestInstance test(backend);
    slag::GraphicsDevice* device = test.firstDevice();
    REQUIRE(device);
    REQUIRE(static_cast<bool>(device->initialize({})));
    {
        TriangleScene scene(device);
        slag::Pipeline* pipeline = scene.pipeline(4);
        REQUIRE(pipeline);
        constexpr uint32_t size = 64;
        slag::Texture* msaa = renderTarget(device, size, 4);
        slag::Texture* commandResolved = renderTarget(device, size, 1);
        slag::AttachmentView* msaaView = device->createAttachmentView({.texture = msaa}).value();
        REQUIRE(msaaView);
        slag::CopyFootprint footprint = device->copyFootprint(commandResolved, 0);
        slag::Buffer* readback =
            device
                ->createBuffer(
                    {.size = footprint.size, .placement = slag::Placement::automatic(slag::MemoryKind::Readback)})
                .value();
        REQUIRE(readback);

        slag::CommandBuffer* commandBuffer =
            device->createCommandBuffer({.queueType = slag::CommandQueueType::Graphics}).value();
        REQUIRE(static_cast<bool>(commandBuffer->begin()));
        slag::TextureBarrier start[] = {
            transition(msaa, slag::TextureLayout::Undefined, slag::TextureLayout::RenderTarget),
            transition(commandResolved, slag::TextureLayout::Undefined, slag::TextureLayout::ResolveDest)};
        commandBuffer->barrier({}, {}, start);
        slag::ColorAttachment color = {.view = msaaView,
                                       .loadOp = slag::LoadOp::Clear,
                                       .storeOp = slag::StoreOp::Store,
                                       .clearValue = {.color = {0.0f, 0.0f, 1.0f, 1.0f}}};
        commandBuffer->beginRendering({.colorAttachments = {&color, 1}});
        scene.draw(commandBuffer, pipeline, 0);
        commandBuffer->endRendering();
        slag::TextureBarrier toResolve =
            transition(msaa, slag::TextureLayout::RenderTarget, slag::TextureLayout::ResolveSource);
        commandBuffer->barrier({}, {}, {&toResolve, 1});
        commandBuffer->resolveTexture(commandResolved, 0, 0, msaa, 0);
        slag::TextureBarrier toCopy =
            transition(commandResolved, slag::TextureLayout::ResolveDest, slag::TextureLayout::CopySource);
        commandBuffer->barrier({}, {}, {&toCopy, 1});
        commandBuffer->copyTextureToBuffer({readback, 0, footprint.rowPitch}, {.texture = commandResolved}, size, size);
        REQUIRE(static_cast<bool>(commandBuffer->end()));
        submitAndWait(device, commandBuffer);

        const auto* a = static_cast<const uint8_t*>(readback->mappedData());
        CHECK(a[8 * footprint.rowPitch + 8 * 4] == 255);       // red inside
        CHECK(a[56 * footprint.rowPitch + 56 * 4 + 2] == 255); // blue outside
        uint32_t averaged = 0;
        for (uint32_t y = 0; y < size; ++y)
        {
            for (uint32_t x = 0; x < size; ++x)
            {
                uint8_t red = a[y * footprint.rowPitch + x * 4];
                averaged += red != 0 && red != 255 ? 1 : 0;
            }
        }
        CHECK(averaged > 0); // edge pixels mix the triangle and the clear color

        device->destroy(commandBuffer);
        device->destroy(readback);
        device->destroy(msaaView);
        device->destroy(msaa);
        device->destroy(commandResolved);
        device->destroy(pipeline);
    }
    device->shutdown();
    test.checkNoErrors();
}

// One render pass across two command buffers: cleared once, a triangle in each.
void testSplitPass(slag::Backend backend)
{
    TestInstance test(backend);
    slag::GraphicsDevice* device = test.firstDevice();
    REQUIRE(device);
    REQUIRE(static_cast<bool>(device->initialize({})));
    {
        TriangleScene scene(device);
        slag::Pipeline* pipeline = scene.pipeline(1);
        REQUIRE(pipeline);
        constexpr uint32_t size = 64;
        slag::Texture* target = renderTarget(device, size, 1);
        slag::AttachmentView* view = device->createAttachmentView({.texture = target}).value();
        slag::CopyFootprint footprint = device->copyFootprint(target, 0);
        slag::Buffer* readback =
            device
                ->createBuffer(
                    {.size = footprint.size, .placement = slag::Placement::automatic(slag::MemoryKind::Readback)})
                .value();
        REQUIRE(view);
        REQUIRE(readback);

        const slag::ColorAttachment color = {
            .view = view, .loadOp = slag::LoadOp::Clear, .clearValue = {.color = {0.0f, 0.0f, 1.0f, 1.0f}}};
        slag::CommandBuffer* first =
            device->createCommandBuffer({.queueType = slag::CommandQueueType::Graphics}).value();
        slag::CommandBuffer* second =
            device->createCommandBuffer({.queueType = slag::CommandQueueType::Graphics}).value();
        REQUIRE(static_cast<bool>(first->begin()));
        slag::TextureBarrier toTarget =
            transition(target, slag::TextureLayout::Undefined, slag::TextureLayout::RenderTarget);
        first->barrier({}, {}, {&toTarget, 1});
        // A label may span the lists of a split render pass (ADR 0050).
        const float labelColor[4] = {0.2f, 0.6f, 1.0f, 1.0f};
        first->beginLabel("split pass", labelColor);
        first->beginRendering({.colorAttachments = {&color, 1}, .suspending = true});
        scene.draw(first, pipeline, 0);
        first->endRendering();
        REQUIRE(static_cast<bool>(first->end()));
        REQUIRE(static_cast<bool>(second->begin()));
        second->beginRendering({.colorAttachments = {&color, 1}, .resuming = true});
        second->insertLabel("second triangle");
        scene.draw(second, pipeline, 1);
        second->endRendering();
        second->endLabel();
        slag::TextureBarrier toCopy =
            transition(target, slag::TextureLayout::RenderTarget, slag::TextureLayout::CopySource);
        second->barrier({}, {}, {&toCopy, 1});
        second->copyTextureToBuffer({readback, 0, footprint.rowPitch}, {.texture = target}, size, size);
        REQUIRE(static_cast<bool>(second->end()));

        slag::Semaphore* semaphore = device->createSemaphore({}).value();
        slag::SemaphoreValue done = {semaphore, 1};
        slag::CommandBuffer* commandBuffers[] = {first, second};
        REQUIRE(static_cast<bool>(device->commandQueue(slag::CommandQueueType::Graphics)
                                      ->submit({.commandBuffers = commandBuffers, .signals = {&done, 1}})));
        REQUIRE(static_cast<bool>(device->waitSemaphores({&done, 1}, slag::WaitMode::All, 5'000'000'000)));

        const auto* pixels = static_cast<const uint8_t*>(readback->mappedData());
        auto at = [&](uint32_t x, uint32_t y) {
            return pixels + y * footprint.rowPitch + x * 4;
        };
        CHECK(at(8, 8)[0] == 255);   // triangle 0, red
        CHECK(at(56, 56)[1] == 255); // triangle 1, green
        CHECK(at(56, 8)[2] == 255);  // cleared once, blue
        CHECK(at(8, 56)[2] == 255);

        device->destroy(semaphore);
        device->destroy(first);
        device->destroy(second);
        device->destroy(readback);
        device->destroy(view);
        device->destroy(target);
        device->destroy(pipeline);
    }
    device->shutdown();
    test.checkNoErrors();
}

// Read-only depth with writable stencil (open item 11): a depth-only
// pipeline tests depth (0.25 < 0.5 passes) without writing it, and replaces
// stencil with 7. The depth plane is in DepthStencilRead, stencil in
// DepthStencilWrite.
void testMixedDepthStencil(slag::Backend backend)
{
    TestInstance test(backend);
    slag::GraphicsDevice* device = test.firstDevice();
    REQUIRE(device);
    REQUIRE(static_cast<bool>(device->initialize({})));
    {
        ShaderFiles vertexShader("FullScreen", "vertexMain");
        slag::DepthStencilState depthStencil = {
            .depthTest = true, .depthWrite = false, .depthCompareOp = slag::CompareOp::Less, .stencilTest = true};
        depthStencil.front = {.passOp = slag::StencilOp::Replace, .compareOp = slag::CompareOp::Always};
        depthStencil.back = depthStencil.front;
        slag::Pipeline* pipeline =
            device
                ->createGraphicsPipeline({.vertex = {.binaries = vertexShader.binaries, .entryPoint = "vertexMain"},
                                          .depthStencil = depthStencil,
                                          .depthStencilFormat = slag::PixelFormat::D32FloatS8Uint})
                .value();
        REQUIRE(pipeline);
        constexpr uint32_t size = 32;
        slag::Texture* texture = device
                                     ->createTexture2D({.format = slag::PixelFormat::D32FloatS8Uint,
                                                        .width = size,
                                                        .height = size,
                                                        .usage = slag::TextureUsage::DepthStencil})
                                     .value();
        REQUIRE(texture);
        slag::AttachmentView* view = device->createAttachmentView({.texture = texture}).value();
        REQUIRE(view);
        slag::CopyFootprint depthFootprint = device->copyFootprint(texture, 0, slag::TexturePlane::Depth);
        slag::CopyFootprint stencilFootprint = device->copyFootprint(texture, 0, slag::TexturePlane::Stencil);
        uint64_t stencilOffset = (depthFootprint.size + 511) / 512 * 512;
        slag::Buffer* readback =
            device
                ->createBuffer({.size = stencilOffset + stencilFootprint.size,
                                .placement = slag::Placement::automatic(slag::MemoryKind::Readback)})
                .value();
        REQUIRE(readback);

        slag::CommandBuffer* commandBuffer =
            device->createCommandBuffer({.queueType = slag::CommandQueueType::Graphics}).value();
        REQUIRE(static_cast<bool>(commandBuffer->begin()));
        // Clear both planes in a writable pass, then switch depth to read-only.
        slag::TextureBarrier toWrite = {.syncBefore = slag::BarrierSync::None,
                                        .syncAfter = slag::BarrierSync::DepthStencil,
                                        .accessAfter = slag::BarrierAccess::DepthStencilWrite,
                                        .layoutBefore = slag::TextureLayout::Undefined,
                                        .layoutAfter = slag::TextureLayout::DepthStencilWrite,
                                        .texture = texture};
        commandBuffer->barrier({}, {}, {&toWrite, 1});
        slag::DepthStencilAttachment clearPass = {.view = view,
                                                  .depthLoadOp = slag::LoadOp::Clear,
                                                  .stencilLoadOp = slag::LoadOp::Clear,
                                                  .clearValue = {.depth = 0.5f, .stencil = 0}};
        commandBuffer->beginRendering({.depthStencilAttachment = &clearPass});
        commandBuffer->endRendering();
        slag::TextureBarrier depthToRead = {.syncBefore = slag::BarrierSync::DepthStencil,
                                            .syncAfter = slag::BarrierSync::DepthStencil,
                                            .accessBefore = slag::BarrierAccess::DepthStencilWrite,
                                            .accessAfter = slag::BarrierAccess::DepthStencilRead,
                                            .layoutBefore = slag::TextureLayout::DepthStencilWrite,
                                            .layoutAfter = slag::TextureLayout::DepthStencilRead,
                                            .texture = texture,
                                            .range = {.plane = slag::TexturePlane::Depth}};
        commandBuffer->barrier({}, {}, {&depthToRead, 1});
        slag::DepthStencilAttachment mixedPass = {.view = view, .readOnlyDepth = true};
        commandBuffer->beginRendering({.depthStencilAttachment = &mixedPass});
        commandBuffer->bindPipeline(pipeline);
        commandBuffer->setStencilReference(7);
        commandBuffer->draw(3, 1);
        commandBuffer->endRendering();
        slag::TextureBarrier toCopy[] = {{.syncBefore = slag::BarrierSync::DepthStencil,
                                          .syncAfter = slag::BarrierSync::Copy,
                                          .accessBefore = slag::BarrierAccess::DepthStencilRead,
                                          .accessAfter = slag::BarrierAccess::CopySource,
                                          .layoutBefore = slag::TextureLayout::DepthStencilRead,
                                          .layoutAfter = slag::TextureLayout::CopySource,
                                          .texture = texture,
                                          .range = {.plane = slag::TexturePlane::Depth}},
                                         {.syncBefore = slag::BarrierSync::DepthStencil,
                                          .syncAfter = slag::BarrierSync::Copy,
                                          .accessBefore = slag::BarrierAccess::DepthStencilWrite,
                                          .accessAfter = slag::BarrierAccess::CopySource,
                                          .layoutBefore = slag::TextureLayout::DepthStencilWrite,
                                          .layoutAfter = slag::TextureLayout::CopySource,
                                          .texture = texture,
                                          .range = {.plane = slag::TexturePlane::Stencil}}};
        commandBuffer->barrier({}, {}, toCopy);
        commandBuffer->copyTextureToBuffer({readback, 0, depthFootprint.rowPitch},
                                           {.texture = texture, .plane = slag::TexturePlane::Depth}, size, size);
        commandBuffer->copyTextureToBuffer({readback, stencilOffset, stencilFootprint.rowPitch},
                                           {.texture = texture, .plane = slag::TexturePlane::Stencil}, size, size);
        REQUIRE(static_cast<bool>(commandBuffer->end()));
        submitAndWait(device, commandBuffer);

        const auto* bytes = static_cast<const uint8_t*>(readback->mappedData());
        float depth = 0.0f;
        std::memcpy(&depth, bytes + 16 * depthFootprint.rowPitch + 16 * 4, 4);
        CHECK(depth == 0.5f);                                                   // not written
        CHECK(bytes[stencilOffset + 16 * stencilFootprint.rowPitch + 16] == 7); // replaced
        CHECK(bytes[stencilOffset + 0 * stencilFootprint.rowPitch + 0] == 7);

        device->destroy(commandBuffer);
        device->destroy(readback);
        device->destroy(view);
        device->destroy(texture);
        device->destroy(pipeline);
    }
    device->shutdown();
    test.checkNoErrors();
}

} // namespace

#ifdef SLAG_BACKEND_VULKAN
TEST_CASE("Vulkan: read-only depth with writable stencil")
{
    testMixedDepthStencil(slag::Backend::Vulkan);
}
#endif

#ifdef SLAG_BACKEND_D3D12
TEST_CASE("D3D12: read-only depth with writable stencil")
{
    testMixedDepthStencil(slag::Backend::D3D12);
}
#endif

#ifdef SLAG_BACKEND_VULKAN
TEST_CASE("Vulkan: MSAA resolves with resolveTexture")
{
    testResolves(slag::Backend::Vulkan);
}

TEST_CASE("Vulkan: a render pass split across two command buffers")
{
    testSplitPass(slag::Backend::Vulkan);
}
#endif

#ifdef SLAG_BACKEND_D3D12
TEST_CASE("D3D12: MSAA resolves with resolveTexture")
{
    testResolves(slag::Backend::D3D12);
}

TEST_CASE("D3D12: a render pass split across two command buffers")
{
    testSplitPass(slag::Backend::D3D12);
}
#endif

#ifdef SLAG_BACKEND_VULKAN
TEST_CASE("Vulkan: indirect draws with a GPU count and draw IDs")
{
    testDrawIds(slag::Backend::Vulkan);
}
#endif

#ifdef SLAG_BACKEND_D3D12
TEST_CASE("D3D12: indirect draws with a GPU count and draw IDs")
{
    testDrawIds(slag::Backend::D3D12);
}
#endif

#ifdef SLAG_BACKEND_VULKAN
TEST_CASE("Vulkan: a triangle renders with +Y up into a cleared target")
{
    testTriangle(slag::Backend::Vulkan);
}
#endif

#ifdef SLAG_BACKEND_D3D12
TEST_CASE("D3D12: a triangle renders with +Y up into a cleared target")
{
    testTriangle(slag::Backend::D3D12);
}
#endif
