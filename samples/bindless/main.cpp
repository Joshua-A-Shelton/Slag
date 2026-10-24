// About 1000 quads in one instanced draw, each picking its texture by heap
// index. Textures are streamed: every few frames one is replaced by a new
// texture in a newly allocated slot, and the old texture and slot are
// released once the GPU is done with them, through DescriptorAllocator and
// DeferredDestroyer (ADRs 0032, 0069).

#include "SampleApp.h"

#include <slag/util/DeferredDestroyer.h>
#include <slag/util/DescriptorAllocator.h>

#include <cstdio>
#include <cstring>

namespace {

constexpr uint32_t columns = 32;
constexpr uint32_t quadCount = columns * columns;
constexpr uint32_t textureCount = 64;
constexpr uint32_t textureSize = 32;
constexpr uint32_t framesPerReplacement = 4;
constexpr uint32_t samplerSlot = 0;
constexpr uint32_t framesInFlight = 2;

struct StreamedTexture
{
    slag::Texture* texture = nullptr;
    uint32_t slot = 0;
};

// A ring or checker pattern in a color that changes with each generation.
void fillTexels(uint8_t* destination, uint32_t rowPitch, uint32_t generation)
{
    float hue = static_cast<float>(generation % 37) / 37.0f * 6.0f;
    auto channel = [&](float offset) {
        float h = hue + offset;
        h = h >= 6.0f ? h - 6.0f : h;
        float v = h < 1.0f ? h : h < 3.0f ? 1.0f : h < 4.0f ? 4.0f - h : 0.0f;
        return static_cast<uint8_t>(40.0f + v * 215.0f);
    };
    uint8_t r = channel(0.0f), g = channel(4.0f), b = channel(2.0f);
    bool rings = generation % 2 == 0;
    for (uint32_t y = 0; y < textureSize; ++y)
    {
        for (uint32_t x = 0; x < textureSize; ++x)
        {
            int dx = static_cast<int>(x) - 16, dy = static_cast<int>(y) - 16;
            bool on = rings ? ((dx * dx + dy * dy) / 40) % 2 == 0 : ((x / 8) + (y / 8)) % 2 == 0;
            uint8_t* texel = destination + y * rowPitch + x * 4;
            texel[0] = on ? r : r / 4;
            texel[1] = on ? g : g / 4;
            texel[2] = on ? b : b / 4;
            texel[3] = 255;
        }
    }
}

} // namespace

int main(int argc, char** argv)
{
    SampleApp app("Slag bindless", parseOptions(argc, argv));
    if (!app.ok())
    {
        return 1;
    }
    slag::GraphicsDevice* device = app.device;

    SampleShader vertexShader("Bindless", "vertexMain");
    SampleShader pixelShader("Bindless", "pixelMain");
    const slag::PixelFormat colorFormats[] = {app.colorFormat};
    slag::Pipeline* pipeline = device
                                   ->createGraphicsPipeline({.vertex = vertexShader.stage("vertexMain"),
                                                             .pixel = pixelShader.stage("pixelMain"),
                                                             .colorFormats = colorFormats,
                                                             .debugName = "bindless quads"})
                                   .value();
    // Per-quad texture indices, one block per frame in flight.
    slag::Buffer* instances = device
                                  ->createBuffer({.size = quadCount * 4 * framesInFlight,
                                                  .placement = slag::Placement::automatic(slag::MemoryKind::Upload),
                                                  .debugName = "instances"})
                                  .value();
    if (!pipeline || !instances)
    {
        return 1;
    }

    slag::DescriptorAllocator slots(device, {.heap = slag::DescriptorHeap::Resource, .firstSlot = 1, .slotCount = 512});
    slag::DeferredDestroyer destroyer(device);
    uint32_t instanceSlots[framesInFlight] = {};
    for (uint32_t i = 0; i < framesInFlight; ++i)
    {
        instanceSlots[i] = slots.allocate();
        device->writeStructuredBuffer(
            instanceSlots[i],
            {.buffer = instances, .offset = uint64_t(i) * quadCount * 4, .count = quadCount, .stride = 4});
    }
    device->writeSampler(samplerSlot, {.minFilter = slag::Filter::Point, .magFilter = slag::Filter::Point});

    StreamedTexture textures[textureCount];
    uint32_t generation = 0;
    uint32_t replaced = 0;
    uint32_t highestSlot = 0;
    bool failed = false;

    // Creates a texture and records its upload into this frame's command
    // buffer; the staging buffer is released with the frame.
    auto stream = [&](slag::CommandBuffer* commandBuffer, StreamedTexture& entry) {
        slag::Texture* texture = device
                                     ->createTexture2D({.format = slag::PixelFormat::R8G8B8A8UnormSrgb,
                                                        .width = textureSize,
                                                        .height = textureSize,
                                                        .debugName = "streamed"})
                                     .value();
        slag::CopyFootprint footprint = texture ? device->copyFootprint(texture, 0) : slag::CopyFootprint{};
        slag::Buffer* staging =
            texture ? device
                          ->createBuffer({.size = footprint.size,
                                          .placement = slag::Placement::automatic(slag::MemoryKind::Upload)})
                          .value()
                    : nullptr;
        uint32_t slot = slots.allocate();
        if (!texture || !staging || slot == 0)
        {
            failed = true;
            device->destroy(staging);
            device->destroy(texture);
            return;
        }
        fillTexels(static_cast<uint8_t*>(staging->mappedData()), footprint.rowPitch, generation++);
        slag::TextureBarrier toCopy = {.syncAfter = slag::BarrierSync::Copy,
                                       .accessAfter = slag::BarrierAccess::CopyDest,
                                       .layoutBefore = slag::TextureLayout::Undefined,
                                       .layoutAfter = slag::TextureLayout::CopyDest,
                                       .texture = texture};
        commandBuffer->barrier({}, {}, {&toCopy, 1});
        commandBuffer->copyBufferToTexture({.texture = texture}, {staging, 0, footprint.rowPitch}, textureSize,
                                           textureSize);
        slag::TextureBarrier toSample = {.syncBefore = slag::BarrierSync::Copy,
                                         .syncAfter = slag::BarrierSync::PixelShading,
                                         .accessBefore = slag::BarrierAccess::CopyDest,
                                         .accessAfter = slag::BarrierAccess::ShaderRead,
                                         .layoutBefore = slag::TextureLayout::CopyDest,
                                         .layoutAfter = slag::TextureLayout::ShaderRead,
                                         .texture = texture};
        commandBuffer->barrier({}, {}, {&toSample, 1});
        // The new slot was never in use, or its last use is complete, so
        // writing it now is safe while earlier frames are still running.
        device->writeTexture(slot, {.texture = texture});

        slag::SemaphoreValue frameDone = app.frameSignal();
        destroyer.destroyAfter(staging, frameDone);
        if (entry.texture)
        {
            // Earlier frames may still sample the old texture through the old slot.
            destroyer.destroyAfter(entry.texture, frameDone);
            slots.release(entry.slot, frameDone);
            ++replaced;
        }
        entry = {texture, slot};
        highestSlot = slot > highestSlot ? slot : highestSlot;
    };

    uint64_t frame = 0;
    int result =
        app.run([&](slag::CommandBuffer* commandBuffer, slag::AttachmentView* target, uint32_t width, uint32_t height) {
            destroyer.collect();
            slots.collect();
            if (frame == 0)
            {
                for (StreamedTexture& entry : textures)
                {
                    stream(commandBuffer, entry);
                }
            }
            else if (frame % framesPerReplacement == 0)
            {
                stream(commandBuffer, textures[(frame / framesPerReplacement) % textureCount]);
            }

            uint32_t block = static_cast<uint32_t>(frame % framesInFlight);
            auto* table = static_cast<uint32_t*>(instances->mappedData()) + block * quadCount;
            for (uint32_t i = 0; i < quadCount; ++i)
            {
                table[i] = textures[(i * 7 + i / columns) % textureCount].slot;
            }

            const slag::ColorAttachment color = {
                .view = target, .loadOp = slag::LoadOp::Clear, .clearValue = {.color = {0.02f, 0.02f, 0.04f, 1.0f}}};
            commandBuffer->beginRendering({.colorAttachments = {&color, 1}});
            commandBuffer->bindPipeline(pipeline);
            struct
            {
                uint32_t instances;
                uint32_t sampler;
                float aspect;
                float time;
                uint32_t columns;
            } push = {instanceSlots[block], samplerSlot,
                      width > height ? static_cast<float>(height) / static_cast<float>(width) : 1.0f,
                      static_cast<float>(frame) / 60.0f, columns};
            commandBuffer->pushConstants(&push, sizeof(push));
            commandBuffer->draw(6, quadCount);
            commandBuffer->endRendering();
            ++frame;
        });

    app.queue->waitIdle();
    destroyer.flush();
    for (StreamedTexture& entry : textures)
    {
        device->destroy(entry.texture);
    }
    device->destroy(instances);
    device->destroy(pipeline);
    std::printf("streamed %u replacement textures; highest slot %u of 512\n", replaced, highestSlot);
    if (failed)
    {
        std::fprintf(stderr, "a texture couldn't be streamed\n");
        return 1;
    }
    return result;
}
