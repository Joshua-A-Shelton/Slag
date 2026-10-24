#include "ImageCheck.h"
#include "TestDevice.h"

#include <slag/AttachmentView.h>
#include <slag/Buffer.h>
#include <slag/CommandBuffer.h>
#include <slag/GraphicsDevice.h>
#include <slag/Pipeline.h>
#include <slag/Semaphore.h>
#include <slag/Texture.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

namespace {

std::vector<char> readFile(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

struct Shader
{
    std::vector<char> spirv;
    std::vector<char> dxil;
    std::vector<slag::ShaderBinary> binaries;

    explicit Shader(const std::string& entry)
    {
        spirv = readFile(std::string(SLAG_SHADER_DIR "/Scene.") + entry + ".spv");
        dxil = readFile(std::string(SLAG_SHADER_DIR "/Scene.") + entry + ".dxil");
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
    float x, y, z;
    float u, v;
    float r, g, b, a;
};

constexpr uint32_t sceneSize = 128;
constexpr uint32_t checkerSize = 8;

// Quads in clip space (+Y up). The green one is in front and drawn first; the
// textured one behind it shows only where they don't overlap.
struct Quad
{
    float left, right, bottom, top, depth;
};
constexpr Quad frontQuad = {-0.5f, 1.0f, -0.5f, 0.75f, 0.3f};
constexpr Quad backQuad = {-1.0f, 0.5f, -1.0f, 0.5f, 0.6f};

uint32_t checker(uint32_t x, uint32_t y)
{
    return (x + y) % 2 ? 0xFFFF4000u : 0xFF0080FFu; // r | g << 8 | b << 16 | a << 24
}

constexpr uint32_t green = 0xFF00FF00u;
constexpr uint32_t clear = 0xFF330000u; // (0, 0, 51/255, 1)

// What the scene must show at pixel (x, y), or nothing near edges.
std::optional<Rgba> expectedPixel(uint32_t x, uint32_t y)
{
    float pixel = 2.0f / sceneSize; // one pixel in clip space
    float cx = (x + 0.5f) * pixel - 1.0f;
    float cy = 1.0f - (y + 0.5f) * pixel;
    auto inside = [&](const Quad& quad, float margin) {
        return cx > quad.left + margin && cx < quad.right - margin && cy > quad.bottom + margin &&
               cy < quad.top - margin;
    };
    auto near = [&](const Quad& quad) {
        return inside(quad, -pixel) && !inside(quad, pixel);
    };
    if (near(frontQuad) || near(backQuad))
    {
        return std::nullopt;
    }
    if (inside(frontQuad, 0.0f))
    {
        return green;
    }
    if (inside(backQuad, 0.0f))
    {
        // Texel coordinates; skip pixels within one pixel of a texel boundary.
        float u = (cx - backQuad.left) / (backQuad.right - backQuad.left) * checkerSize;
        float v = (backQuad.top - cy) / (backQuad.top - backQuad.bottom) * checkerSize;
        float texelPixels = (backQuad.right - backQuad.left) / checkerSize / pixel;
        auto nearBoundary = [&](float t) {
            float f = (t - std::floor(t)) * texelPixels;
            return f < 1.0f || f > texelPixels - 1.0f;
        };
        if (nearBoundary(u) || nearBoundary(v))
        {
            return std::nullopt;
        }
        return checker(static_cast<uint32_t>(u), static_cast<uint32_t>(v));
    }
    return clear;
}

void submitAndWait(slag::GraphicsDevice* device, slag::CommandBuffer* commandBuffer)
{
    slag::Semaphore* semaphore = device->createSemaphore({}).value();
    REQUIRE(semaphore);
    slag::SemaphoreValue done = {semaphore, 1};
    slag::CommandBuffer* commandBuffers[] = {commandBuffer};
    REQUIRE(static_cast<bool>(device->commandQueue(slag::CommandQueueType::Graphics)
                                  ->submit({.commandBuffers = commandBuffers, .signals = {&done, 1}})));
    REQUIRE(static_cast<bool>(done.semaphore->wait(done.value, 5'000'000'000)));
    device->destroy(semaphore);
}

// Renders the textured, depth-tested scene off-screen on the first device.
Image renderScene(slag::Backend backend)
{
    TestInstance test(backend);
    slag::GraphicsDevice* device = test.firstDevice();
    REQUIRE(device);
    MESSAGE("device: " << std::string(device->info().name));
    REQUIRE(static_cast<bool>(device->initialize({})));
    Image image;
    {
        Shader vertexShader("vertexMain");
        Shader pixelShader("pixelMain");
        const slag::VertexAttribute attributes[] = {
            {.location = 0, .format = slag::PixelFormat::R32G32B32Float, .slot = 0, .offset = 0},
            {.location = 1, .format = slag::PixelFormat::R32G32Float, .slot = 0, .offset = 12},
            {.location = 2, .format = slag::PixelFormat::R32G32B32A32Float, .slot = 0, .offset = 20}};
        const slag::VertexStepRate slots[] = {slag::VertexStepRate::PerVertex};
        const slag::PixelFormat colorFormats[] = {slag::PixelFormat::R8G8B8A8Unorm};
        slag::Pipeline* pipeline =
            device
                ->createGraphicsPipeline(
                    {.vertex = {.binaries = vertexShader.binaries, .entryPoint = "vertexMain"},
                     .pixel = {.binaries = pixelShader.binaries, .entryPoint = "pixelMain"},
                     .vertexInput = {.attributes = attributes, .slots = slots},
                     .topology = slag::PrimitiveTopology::TriangleList,
                     .depthStencil = {.depthTest = true, .depthWrite = true, .depthCompareOp = slag::CompareOp::Less},
                     .colorFormats = colorFormats,
                     .depthStencilFormat = slag::PixelFormat::D32Float,
                     .debugName = "scene"})
                .value();
        REQUIRE(pipeline);

        slag::Texture* color = device
                                   ->createTexture2D({.format = slag::PixelFormat::R8G8B8A8Unorm,
                                                      .width = sceneSize,
                                                      .height = sceneSize,
                                                      .usage = slag::TextureUsage::RenderTarget})
                                   .value();
        slag::Texture* depth = device
                                   ->createTexture2D({.format = slag::PixelFormat::D32Float,
                                                      .width = sceneSize,
                                                      .height = sceneSize,
                                                      .usage = slag::TextureUsage::DepthStencil})
                                   .value();
        slag::Texture* checkerTexture =
            device
                ->createTexture2D(
                    {.format = slag::PixelFormat::R8G8B8A8Unorm, .width = checkerSize, .height = checkerSize})
                .value();
        REQUIRE(color);
        REQUIRE(depth);
        REQUIRE(checkerTexture);
        slag::AttachmentView* colorView = device->createAttachmentView({.texture = color}).value();
        slag::AttachmentView* depthView = device->createAttachmentView({.texture = depth}).value();
        REQUIRE(colorView);
        REQUIRE(depthView);

        // Upload buffer: vertices at 0, the tint at 1024, checker texels at 2048 (256-byte rows).
        slag::Buffer* upload =
            device->createBuffer({.size = 8192, .placement = slag::Placement::automatic(slag::MemoryKind::Upload)})
                .value();
        REQUIRE(upload);
        auto* bytes = static_cast<uint8_t*>(upload->mappedData());
        auto quadVertices = [](const Quad& quad, float r, float g, float b, float a, Vertex* out) {
            const Vertex corners[4] = {{quad.left, quad.top, quad.depth, 0, 0, r, g, b, a},
                                       {quad.right, quad.top, quad.depth, 1, 0, r, g, b, a},
                                       {quad.left, quad.bottom, quad.depth, 0, 1, r, g, b, a},
                                       {quad.right, quad.bottom, quad.depth, 1, 1, r, g, b, a}};
            const int order[6] = {0, 1, 2, 1, 3, 2};
            for (int i = 0; i < 6; ++i)
            {
                out[i] = corners[order[i]];
            }
        };
        Vertex vertices[12];
        quadVertices(frontQuad, 0, 1, 0, 1, vertices);
        quadVertices(backQuad, 0, 0, 0, 0, vertices + 6); // alpha 0: textured
        std::memcpy(bytes, vertices, sizeof(vertices));
        const float tint[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        std::memcpy(bytes + 1024, tint, sizeof(tint));
        for (uint32_t y = 0; y < checkerSize; ++y)
        {
            for (uint32_t x = 0; x < checkerSize; ++x)
            {
                uint32_t texel = checker(x, y);
                std::memcpy(bytes + 2048 + y * 256 + x * 4, &texel, 4);
            }
        }
        device->writeTexture(7, {.texture = checkerTexture});
        device->writeSampler(2, {.minFilter = slag::Filter::Point,
                                 .magFilter = slag::Filter::Point,
                                 .mipFilter = slag::Filter::Point,
                                 .addressU = slag::AddressMode::Clamp,
                                 .addressV = slag::AddressMode::Clamp});

        slag::CopyFootprint footprint = device->copyFootprint(color, 0);
        slag::Buffer* readback =
            device
                ->createBuffer(
                    {.size = footprint.size, .placement = slag::Placement::automatic(slag::MemoryKind::Readback)})
                .value();
        REQUIRE(readback);

        slag::CommandBuffer* commandBuffer =
            device->createCommandBuffer({.queueType = slag::CommandQueueType::Graphics}).value();
        REQUIRE(static_cast<bool>(commandBuffer->begin()));
        slag::TextureBarrier start[] = {{.syncBefore = slag::BarrierSync::None,
                                         .syncAfter = slag::BarrierSync::Copy,
                                         .accessAfter = slag::BarrierAccess::CopyDest,
                                         .layoutBefore = slag::TextureLayout::Undefined,
                                         .layoutAfter = slag::TextureLayout::CopyDest,
                                         .texture = checkerTexture},
                                        {.syncBefore = slag::BarrierSync::None,
                                         .syncAfter = slag::BarrierSync::RenderTarget,
                                         .accessAfter = slag::BarrierAccess::RenderTarget,
                                         .layoutBefore = slag::TextureLayout::Undefined,
                                         .layoutAfter = slag::TextureLayout::RenderTarget,
                                         .texture = color},
                                        {.syncBefore = slag::BarrierSync::None,
                                         .syncAfter = slag::BarrierSync::DepthStencil,
                                         .accessAfter = slag::BarrierAccess::DepthStencilWrite,
                                         .layoutBefore = slag::TextureLayout::Undefined,
                                         .layoutAfter = slag::TextureLayout::DepthStencilWrite,
                                         .texture = depth}};
        commandBuffer->barrier({}, {}, start);
        commandBuffer->copyBufferToTexture({.texture = checkerTexture}, {upload, 2048, 256}, checkerSize, checkerSize);
        slag::TextureBarrier toSampled = {.syncBefore = slag::BarrierSync::Copy,
                                          .syncAfter = slag::BarrierSync::PixelShading,
                                          .accessBefore = slag::BarrierAccess::CopyDest,
                                          .accessAfter = slag::BarrierAccess::ShaderRead,
                                          .layoutBefore = slag::TextureLayout::CopyDest,
                                          .layoutAfter = slag::TextureLayout::ShaderRead,
                                          .texture = checkerTexture};
        commandBuffer->barrier({}, {}, {&toSampled, 1});

        const slag::ColorAttachment colorAttachment = {
            .view = colorView, .loadOp = slag::LoadOp::Clear, .clearValue = {.color = {0, 0, 51.0f / 255.0f, 1}}};
        const slag::DepthStencilAttachment depthAttachment = {.view = depthView,
                                                              .depthLoadOp = slag::LoadOp::Clear,
                                                              .depthStoreOp = slag::StoreOp::Discard,
                                                              .clearValue = {.depth = 1.0f}};
        commandBuffer->beginRendering(
            {.colorAttachments = {&colorAttachment, 1}, .depthStencilAttachment = &depthAttachment});
        commandBuffer->bindPipeline(pipeline);
        const uint32_t push[] = {7, 2};
        commandBuffer->pushConstants(push, sizeof(push));
        commandBuffer->setConstantBuffer(upload->gpuAddress() + 1024, 256);
        const slag::VertexBufferBinding binding = {upload, 0, sizeof(vertices), sizeof(Vertex)};
        commandBuffer->bindVertexBuffers(0, {&binding, 1});
        commandBuffer->draw(6, 1, 0); // front first: the depth test hides the back quad behind it
        commandBuffer->draw(6, 1, 6);
        commandBuffer->endRendering();
        slag::TextureBarrier toCopy = {.syncBefore = slag::BarrierSync::RenderTarget,
                                       .syncAfter = slag::BarrierSync::Copy,
                                       .accessBefore = slag::BarrierAccess::RenderTarget,
                                       .accessAfter = slag::BarrierAccess::CopySource,
                                       .layoutBefore = slag::TextureLayout::RenderTarget,
                                       .layoutAfter = slag::TextureLayout::CopySource,
                                       .texture = color};
        commandBuffer->barrier({}, {}, {&toCopy, 1});
        commandBuffer->copyTextureToBuffer({readback, 0, footprint.rowPitch}, {.texture = color}, sceneSize, sceneSize);
        REQUIRE(static_cast<bool>(commandBuffer->end()));
        submitAndWait(device, commandBuffer);

        image.width = sceneSize;
        image.height = sceneSize;
        image.rgba.resize(size_t(sceneSize) * sceneSize * 4);
        for (uint32_t y = 0; y < sceneSize; ++y)
        {
            std::memcpy(image.rgba.data() + size_t(y) * sceneSize * 4,
                        static_cast<const uint8_t*>(readback->mappedData()) + y * footprint.rowPitch, sceneSize * 4);
        }

        device->destroy(commandBuffer);
        device->destroy(readback);
        device->destroy(upload);
        device->destroy(colorView);
        device->destroy(depthView);
        device->destroy(color);
        device->destroy(depth);
        device->destroy(checkerTexture);
        device->destroy(pipeline);
    }
    device->shutdown();
    test.checkNoErrors();
    return image;
}

void testScene(slag::Backend backend)
{
    Image image = renderScene(backend);
    CHECK(checkExact(image, expectedPixel) == 0);
    CHECK(checkGolden(image, "textured_depth_scene"));
}

} // namespace

#ifdef SLAG_BACKEND_VULKAN
TEST_CASE("Vulkan: textured, depth-tested scene (exact and golden checks)")
{
    testScene(slag::Backend::Vulkan);
}
#endif

#ifdef SLAG_BACKEND_D3D12
TEST_CASE("D3D12: textured, depth-tested scene (exact and golden checks)")
{
    testScene(slag::Backend::D3D12);
}
#endif

// Only where both backends have a device (not in CI yet).
#if defined(SLAG_BACKEND_VULKAN) && defined(SLAG_BACKEND_D3D12)
TEST_CASE("D3D12 and Vulkan render the same scene")
{
    Image d3d12 = renderScene(slag::Backend::D3D12);
    Image vulkan = renderScene(slag::Backend::Vulkan);
    std::string why;
    CHECK_MESSAGE(imagesMatch(d3d12, vulkan, why), why);
}
#endif
