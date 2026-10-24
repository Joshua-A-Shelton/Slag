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

uint8_t unorm(float value)
{
    return static_cast<uint8_t>(value * 255.0f + 0.5f);
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
                CHECK(p[0] == unorm((draw + 1) / 8.0f));
                CHECK(p[1] == unorm((instance + 1) / 4.0f));
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

} // namespace

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
