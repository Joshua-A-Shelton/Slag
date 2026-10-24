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

} // namespace

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
