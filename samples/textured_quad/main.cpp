// A spinning, mipmapped, generated texture on an indexed quad: descriptors,
// samplers, staging uploads, and the root constant buffer.

#include "SampleApp.h"

#include <cmath>
#include <cstring>
#include <vector>

namespace {

struct Vertex
{
    float x, y;
    float u, v;
};

constexpr uint32_t textureSize = 256;
constexpr uint32_t mipCount = 9;
constexpr uint32_t textureSlot = 0;
constexpr uint32_t samplerSlot = 0;

// A checkerboard with a color gradient; each mip is a box-filtered copy of
// the one above, so minification shows the mips working.
std::vector<uint8_t> makeMip(uint32_t mip)
{
    uint32_t size = textureSize >> mip;
    std::vector<uint8_t> texels(size_t(size) * size * 4);
    for (uint32_t y = 0; y < size; ++y)
    {
        for (uint32_t x = 0; x < size; ++x)
        {
            uint8_t* texel = texels.data() + (size_t(y) * size + x) * 4;
            if (mip == 0)
            {
                bool dark = ((x / 32) + (y / 32)) % 2 == 1;
                texel[0] = static_cast<uint8_t>(dark ? 40 : 80 + x * 175 / textureSize);
                texel[1] = static_cast<uint8_t>(dark ? 40 : 80 + y * 175 / textureSize);
                texel[2] = static_cast<uint8_t>(dark ? 60 : 200);
                texel[3] = 255;
            }
        }
    }
    return texels;
}

void downsample(const std::vector<uint8_t>& source, uint32_t sourceSize, std::vector<uint8_t>& destination)
{
    uint32_t size = sourceSize / 2;
    for (uint32_t y = 0; y < size; ++y)
    {
        for (uint32_t x = 0; x < size; ++x)
        {
            for (uint32_t c = 0; c < 4; ++c)
            {
                auto at = [&](uint32_t sx, uint32_t sy) {
                    return source[(size_t(sy) * sourceSize + sx) * 4 + c];
                };
                destination[(size_t(y) * size + x) * 4 + c] = static_cast<uint8_t>(
                    (at(2 * x, 2 * y) + at(2 * x + 1, 2 * y) + at(2 * x, 2 * y + 1) + at(2 * x + 1, 2 * y + 1) + 2) /
                    4);
            }
        }
    }
}

} // namespace

int main(int argc, char** argv)
{
    SampleApp app("Slag textured quad", parseOptions(argc, argv));
    if (!app.ok())
    {
        return 1;
    }
    slag::GraphicsDevice* device = app.device;

    SampleShader vertexShader("TexturedQuad", "vertexMain");
    SampleShader pixelShader("TexturedQuad", "pixelMain");
    const slag::VertexAttribute attributes[] = {
        {.location = 0, .format = slag::PixelFormat::R32G32Float, .slot = 0, .offset = 0},
        {.location = 1, .format = slag::PixelFormat::R32G32Float, .slot = 0, .offset = 8}};
    const slag::VertexStepRate slots[] = {slag::VertexStepRate::PerVertex};
    const slag::PixelFormat colorFormats[] = {app.colorFormat};
    slag::Pipeline* pipeline = device
                                   ->createGraphicsPipeline({.vertex = vertexShader.stage("vertexMain"),
                                                             .pixel = pixelShader.stage("pixelMain"),
                                                             .vertexInput = {.attributes = attributes, .slots = slots},
                                                             .colorFormats = colorFormats,
                                                             .debugName = "textured quad"})
                                   .value();
    slag::Texture* texture = device
                                 ->createTexture2D({.format = slag::PixelFormat::R8G8B8A8UnormSrgb,
                                                    .width = textureSize,
                                                    .height = textureSize,
                                                    .mipCount = mipCount,
                                                    .debugName = "generated texture"})
                                 .value();

    // Geometry and every mip go through one staging buffer into GpuOnly memory.
    const Vertex vertices[] = {{-0.7f, 0.7f, 0, 0}, {0.7f, 0.7f, 1, 0}, {0.7f, -0.7f, 1, 1}, {-0.7f, -0.7f, 0, 1}};
    const uint16_t indices[] = {0, 1, 2, 0, 2, 3};
    slag::Buffer* geometry =
        device->createBuffer({.size = 256, .placement = slag::Placement::automatic(slag::MemoryKind::GpuOnly)}).value();
    // Per-frame constants: one 256-byte block per frame in flight.
    slag::Buffer* constants =
        device->createBuffer({.size = 512, .placement = slag::Placement::automatic(slag::MemoryKind::Upload)}).value();
    uint64_t stagingSize = 512;
    for (uint32_t mip = 0; mip < mipCount; ++mip)
    {
        stagingSize += device->copyFootprint(texture, mip).size + 512;
    }
    slag::Buffer* staging =
        device->createBuffer({.size = stagingSize, .placement = slag::Placement::automatic(slag::MemoryKind::Upload)})
            .value();
    if (!pipeline || !texture || !geometry || !constants || !staging)
    {
        return 1;
    }

    auto* bytes = static_cast<uint8_t*>(staging->mappedData());
    std::memcpy(bytes, vertices, sizeof(vertices));
    std::memcpy(bytes + 128, indices, sizeof(indices));
    slag::CommandBuffer* upload = device->createCommandBuffer({.queueType = slag::CommandQueueType::Graphics}).value();
    (void)upload->begin();
    slag::BufferBarrier geometryToCopy = {
        .syncAfter = slag::BarrierSync::Copy, .accessAfter = slag::BarrierAccess::CopyDest, .buffer = geometry};
    slag::TextureBarrier textureToCopy = {.syncAfter = slag::BarrierSync::Copy,
                                          .accessAfter = slag::BarrierAccess::CopyDest,
                                          .layoutBefore = slag::TextureLayout::Undefined,
                                          .layoutAfter = slag::TextureLayout::CopyDest,
                                          .texture = texture};
    upload->barrier({}, {&geometryToCopy, 1}, {&textureToCopy, 1});
    upload->copyBuffer(geometry, 0, staging, 0, 256);
    uint64_t offset = 512;
    std::vector<uint8_t> mip = makeMip(0);
    for (uint32_t level = 0; level < mipCount; ++level)
    {
        if (level > 0)
        {
            std::vector<uint8_t> next(mip.size() / 4);
            downsample(mip, textureSize >> (level - 1), next);
            mip = std::move(next);
        }
        slag::CopyFootprint footprint = device->copyFootprint(texture, level);
        uint32_t size = textureSize >> level;
        for (uint32_t row = 0; row < size; ++row)
        {
            std::memcpy(bytes + offset + row * footprint.rowPitch, mip.data() + size_t(row) * size * 4, size * 4);
        }
        upload->copyBufferToTexture({.texture = texture, .mip = level}, {staging, offset, footprint.rowPitch}, size,
                                    size);
        offset = (offset + footprint.size + 511) / 512 * 512;
    }
    slag::BufferBarrier geometryToDraw = {.syncBefore = slag::BarrierSync::Copy,
                                          .syncAfter = slag::BarrierSync::IndexInput,
                                          .accessBefore = slag::BarrierAccess::CopyDest,
                                          .accessAfter =
                                              slag::BarrierAccess::VertexBuffer | slag::BarrierAccess::IndexBuffer,
                                          .buffer = geometry};
    slag::TextureBarrier textureToSample = {.syncBefore = slag::BarrierSync::Copy,
                                            .syncAfter = slag::BarrierSync::PixelShading,
                                            .accessBefore = slag::BarrierAccess::CopyDest,
                                            .accessAfter = slag::BarrierAccess::ShaderRead,
                                            .layoutBefore = slag::TextureLayout::CopyDest,
                                            .layoutAfter = slag::TextureLayout::ShaderRead,
                                            .texture = texture};
    upload->barrier({}, {&geometryToDraw, 1}, {&textureToSample, 1});
    (void)upload->end();
    if (!app.submitAndWait(upload))
    {
        return 1;
    }
    device->destroy(upload);
    device->destroy(staging);

    device->writeTexture(textureSlot, {.texture = texture});
    device->writeSampler(samplerSlot, {.maxAnisotropy = 8});

    uint64_t frame = 0;
    int result =
        app.run([&](slag::CommandBuffer* commandBuffer, slag::AttachmentView* target, uint32_t width, uint32_t height) {
            // This frame's constants; the other block may still be in use.
            uint64_t block = (frame++ % 2) * 256;
            float angle = static_cast<float>(frame) * 0.01f;
            std::memcpy(static_cast<uint8_t*>(constants->mappedData()) + block, &angle, sizeof(angle));

            const slag::ColorAttachment color = {
                .view = target, .loadOp = slag::LoadOp::Clear, .clearValue = {.color = {0.02f, 0.02f, 0.04f, 1.0f}}};
            commandBuffer->beginRendering({.colorAttachments = {&color, 1}});
            commandBuffer->bindPipeline(pipeline);
            struct
            {
                uint32_t texture;
                uint32_t sampler;
                float aspect;
            } push = {textureSlot, samplerSlot, static_cast<float>(height) / static_cast<float>(width)};
            commandBuffer->pushConstants(&push, sizeof(push));
            commandBuffer->setConstantBuffer(constants->gpuAddress() + block, 256);
            const slag::VertexBufferBinding binding = {geometry, 0, sizeof(vertices), sizeof(Vertex)};
            commandBuffer->bindVertexBuffers(0, {&binding, 1});
            commandBuffer->bindIndexBuffer(geometry, 128, sizeof(indices), slag::IndexFormat::Uint16);
            commandBuffer->drawIndexed(6, 1);
            commandBuffer->endRendering();
        });

    (void)app.queue->waitIdle();
    device->destroy(geometry);
    device->destroy(constants);
    device->destroy(texture);
    device->destroy(pipeline);
    return result;
}
