// Three queues chained by semaphores: the compute queue moves particles, the
// copy queue uploads a texture, and the graphics queue draws the particles,
// colored by the texture, into an offscreen target. Graphics frame f uses
// what compute and copy made in frame f - 1, so all three queues can run at
// once; the particle pass is its own submit, so it doesn't wait for the
// swapchain image, and the frame's swapchain pass only draws the target to
// the screen. Timestamps on each queue, put on the CPU clock by calibration,
// show how much the queues overlapped.

#include "SampleApp.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {

constexpr uint32_t textureSize = 1024;
constexpr uint32_t targetSize = 1024;     // the square offscreen target
constexpr uint32_t stagingCount = 4;      // texture contents, cycled
constexpr uint32_t framesPerContent = 30; // frames each content is uploaded for
constexpr uint32_t framesInFlight = 2;
constexpr uint32_t warmupFrames = 10; // not measured
constexpr uint32_t samplerSlot = 0;

struct Particle
{
    float x, y;
    float vx, vy;
};

enum Queue : uint32_t
{
    GraphicsQueue,
    ComputeQueue,
    CopyQueue,
    QueueCount,
};

const char* const queueNames[QueueCount] = {"graphics", "compute", "copy"};

// GPU ticks on one queue to CPU nanoseconds (ADR 0051).
struct Clock
{
    slag::TimestampCalibration calibration;
    double nanosecondsPerTick = 0.0;

    int64_t toCpu(uint64_t ticks) const
    {
        int64_t delta = static_cast<int64_t>(ticks - calibration.gpuTimestamp);
        return static_cast<int64_t>(calibration.cpuNanoseconds) +
               static_cast<int64_t>(static_cast<double>(delta) * nanosecondsPerTick);
    }
};

struct Interval
{
    int64_t start, end;
};

// Total time each interval in `work` overlapped any interval in `graphics`
// (sorted and disjoint: one queue's work runs in order).
int64_t overlap(const std::vector<Interval>& work, const std::vector<Interval>& graphics)
{
    int64_t total = 0;
    for (const Interval& interval : work)
    {
        auto it = std::lower_bound(graphics.begin(), graphics.end(), interval.start,
                                   [](const Interval& g, int64_t start) { return g.end <= start; });
        for (; it != graphics.end() && it->start < interval.end; ++it)
        {
            total += std::min(interval.end, it->end) - std::max(interval.start, it->start);
        }
    }
    return total;
}

void fillContent(uint8_t* destination, uint32_t rowPitch, uint32_t index)
{
    for (uint32_t y = 0; y < textureSize; ++y)
    {
        for (uint32_t x = 0; x < textureSize; ++x)
        {
            uint8_t* texel = destination + size_t(y) * rowPitch + x * 4;
            uint32_t band = (x / 128 + y / 128 + index) % 3;
            texel[0] = static_cast<uint8_t>(band == 0 ? 255 : 60 + x * 120 / textureSize);
            texel[1] = static_cast<uint8_t>(band == 1 ? 255 : 60 + y * 120 / textureSize);
            texel[2] = static_cast<uint8_t>(band == 2 ? 255 : 120);
            texel[3] = 255;
        }
    }
}

} // namespace

int main(int argc, char** argv)
{
    SampleApp app("Slag multi-queue", parseOptions(argc, argv));
    if (!app.ok())
    {
        return 1;
    }
    // Overlap only shows when the GPU is busy, not waiting for vsync.
    const slag::PresentMode presentModes[] = {slag::PresentMode::Mailbox, slag::PresentMode::Immediate};
    app.preferPresentModes(presentModes);
    slag::GraphicsDevice* device = app.device;
    // Software devices (WARP in CI) get a lighter load.
    const uint32_t particleCount =
        device->info().deviceType == slag::GraphicsDeviceType::Software ? 1u << 12 : 1u << 18;
    const slag::GraphicsDeviceCapabilities& capabilities = device->capabilities();
    slag::CommandQueue* queues[QueueCount] = {app.queue, device->commandQueue(slag::CommandQueueType::Compute),
                                              device->commandQueue(slag::CommandQueueType::Copy)};
    const slag::CommandQueueType queueTypes[QueueCount] = {
        slag::CommandQueueType::Graphics, slag::CommandQueueType::Compute, slag::CommandQueueType::Copy};

    SampleShader updateShader("ParticleUpdate", "computeMain");
    SampleShader vertexShader("Particles", "vertexMain");
    SampleShader pixelShader("Particles", "pixelMain");
    SampleShader compositeVertex("FullScreenTexture", "vertexMain");
    SampleShader compositePixel("FullScreenTexture", "pixelMain");
    slag::Pipeline* updatePipeline =
        device->createComputePipeline({.compute = updateShader.stage("computeMain"), .debugName = "particle update"})
            .value();
    const slag::PixelFormat targetFormats[] = {slag::PixelFormat::R16G16B16A16Float};
    slag::GraphicsPipelineDesc drawDesc = {.vertex = vertexShader.stage("vertexMain"),
                                           .pixel = pixelShader.stage("pixelMain"),
                                           .colorFormats = targetFormats,
                                           .debugName = "particles"};
    drawDesc.blend.targets[0] = {
        .blendEnable = true, .srcColorFactor = slag::BlendFactor::One, .destColorFactor = slag::BlendFactor::One};
    slag::Pipeline* drawPipeline = device->createGraphicsPipeline(drawDesc).value();
    const slag::PixelFormat colorFormats[] = {app.colorFormat};
    slag::Pipeline* compositePipeline = device
                                            ->createGraphicsPipeline({.vertex = compositeVertex.stage("vertexMain"),
                                                                      .pixel = compositePixel.stage("pixelMain"),
                                                                      .colorFormats = colorFormats,
                                                                      .debugName = "composite"})
                                            .value();
    slag::Texture* target = device
                                ->createTexture2D({.format = slag::PixelFormat::R16G16B16A16Float,
                                                   .width = targetSize,
                                                   .height = targetSize,
                                                   .usage = slag::TextureUsage::RenderTarget,
                                                   .debugName = "particle target"})
                                .value();
    slag::AttachmentView* targetView = target ? device->createAttachmentView({.texture = target}).value() : nullptr;
    // Particles ping-pong between two buffers; textures between two too.
    slag::Buffer* particles[2] = {};
    slag::Texture* textures[2] = {};
    for (uint32_t i = 0; i < 2; ++i)
    {
        particles[i] = device
                           ->createBuffer({.size = particleCount * sizeof(Particle),
                                           .placement = slag::Placement::automatic(slag::MemoryKind::GpuOnly),
                                           .debugName = "particles"})
                           .value();
        textures[i] = device
                          ->createTexture2D({.format = slag::PixelFormat::R8G8B8A8UnormSrgb,
                                             .width = textureSize,
                                             .height = textureSize,
                                             .debugName = "uploaded"})
                          .value();
    }
    slag::CopyFootprint footprint = device->copyFootprint(textures[0], 0);
    slag::Buffer* staging = device
                                ->createBuffer({.size = footprint.size * stagingCount,
                                                .placement = slag::Placement::automatic(slag::MemoryKind::Upload),
                                                .debugName = "texture contents"})
                                .value();
    slag::Buffer* particleStaging =
        device
            ->createBuffer({.size = particleCount * sizeof(Particle),
                            .placement = slag::Placement::automatic(slag::MemoryKind::Upload)})
            .value();
    slag::Semaphore* computeDone = device->createSemaphore({.debugName = "compute done"}).value();
    slag::Semaphore* copyDone = device->createSemaphore({.debugName = "copy done"}).value();
    slag::Semaphore* particlesDrawn = device->createSemaphore({.debugName = "particles drawn"}).value();
    if (!updatePipeline || !drawPipeline || !compositePipeline || !targetView || !particles[0] || !particles[1] ||
        !textures[0] || !textures[1] || !staging || !particleStaging || !computeDone || !copyDone || !particlesDrawn)
    {
        return 1;
    }

    // Timestamps: two per frame in flight on each queue that has them.
    bool timed[QueueCount] = {true, true, capabilities.copyQueueTimestamps};
    bool calibrated = capabilities.timestampCalibration;
    slag::QueryPool* pools[QueueCount] = {};
    slag::Buffer* results[QueueCount] = {};
    Clock clocks[QueueCount] = {};
    std::vector<Interval> intervals[QueueCount];
    for (uint32_t q = 0; q < QueueCount; ++q)
    {
        if (!timed[q])
        {
            continue;
        }
        pools[q] = device->createQueryPool({.count = 2 * framesInFlight, .queueType = queueTypes[q]}).value();
        results[q] = device
                         ->createBuffer({.size = 2 * framesInFlight * sizeof(uint64_t),
                                         .placement = slag::Placement::automatic(slag::MemoryKind::Readback)})
                         .value();
        if (!pools[q] || !results[q])
        {
            return 1;
        }
        clocks[q].nanosecondsPerTick = 1e9 / static_cast<double>(queues[q]->timestampFrequency());
        if (calibrated)
        {
            slag::Result<slag::TimestampCalibration> calibration = queues[q]->calibrateTimestamps();
            calibrated = calibration.status() == slag::Status::Ok;
            clocks[q].calibration = calibration ? calibration.value() : slag::TimestampCalibration{};
        }
    }

    slag::CommandBuffer* computeLists[framesInFlight] = {};
    slag::CommandBuffer* copyLists[framesInFlight] = {};
    slag::CommandBuffer* drawLists[framesInFlight] = {};
    for (uint32_t i = 0; i < framesInFlight; ++i)
    {
        computeLists[i] = device->createCommandBuffer({.queueType = slag::CommandQueueType::Compute}).value();
        copyLists[i] = device->createCommandBuffer({.queueType = slag::CommandQueueType::Copy}).value();
        drawLists[i] = device->createCommandBuffer({.queueType = slag::CommandQueueType::Graphics}).value();
        if (!computeLists[i] || !copyLists[i] || !drawLists[i])
        {
            return 1;
        }
    }

    // Descriptors: particle buffers as read and read-write, both textures.
    enum : uint32_t
    {
        ParticlesRead = 0, // + buffer index
        ParticlesWrite = 2,
        TextureSlots = 4, // + texture index
        TargetSlot = 6,
    };
    for (uint32_t i = 0; i < 2; ++i)
    {
        slag::StructuredBufferDescriptor buffer = {
            .buffer = particles[i], .offset = 0, .count = particleCount, .stride = sizeof(Particle)};
        device->writeStructuredBuffer(ParticlesRead + i, buffer);
        device->writeRWStructuredBuffer(ParticlesWrite + i, buffer);
        device->writeTexture(TextureSlots + i, {.texture = textures[i]});
    }
    device->writeTexture(TargetSlot, {.texture = target});
    device->writeSampler(samplerSlot, {.addressU = slag::AddressMode::Clamp, .addressV = slag::AddressMode::Clamp});

    // Startup, on the graphics queue: the first particles go into buffer 1
    // (compute frame 0 reads it), and the textures move to Common, the
    // layout copy queues use (ADR 0049).
    auto* initial = static_cast<Particle*>(particleStaging->mappedData());
    uint32_t seed = 12345;
    auto random = [&] {
        seed = seed * 1664525u + 1013904223u;
        return static_cast<float>(seed >> 8) / static_cast<float>(1u << 24) * 2.0f - 1.0f;
    };
    for (uint32_t i = 0; i < particleCount; ++i)
    {
        initial[i] = {random(), random(), 0.0f, 0.0f};
    }
    for (uint32_t i = 0; i < stagingCount; ++i)
    {
        fillContent(static_cast<uint8_t*>(staging->mappedData()) + i * footprint.size, footprint.rowPitch, i);
    }
    slag::CommandBuffer* setup = device->createCommandBuffer({}).value();
    setup->begin();
    setup->copyBuffer(particles[1], 0, particleStaging, 0, particleCount * sizeof(Particle));
    slag::TextureBarrier toCommon[2];
    for (uint32_t i = 0; i < 2; ++i)
    {
        toCommon[i] = {.layoutBefore = slag::TextureLayout::Undefined,
                       .layoutAfter = slag::TextureLayout::Common,
                       .texture = textures[i]};
    }
    setup->barrier({}, {}, toCommon);
    // The composite pass reads the target in ShaderRead between frames.
    slag::TextureBarrier targetToRead = {.layoutBefore = slag::TextureLayout::Undefined,
                                         .layoutAfter = slag::TextureLayout::ShaderRead,
                                         .texture = target};
    setup->barrier({}, {}, {&targetToRead, 1});
    setup->end();
    if (!app.submitAndWait(setup))
    {
        return 1;
    }
    device->destroy(setup);
    device->destroy(particleStaging);

    // Reads frame f's timestamps on one queue once that frame is done there.
    uint64_t frame = 0;
    auto collect = [&](uint32_t q, uint64_t f) {
        if (!timed[q] || !calibrated || f < warmupFrames)
        {
            return;
        }
        const auto* values = static_cast<const uint64_t*>(results[q]->mappedData()) + (f % framesInFlight) * 2;
        intervals[q].push_back({clocks[q].toCpu(values[0]), clocks[q].toCpu(values[1])});
    };
    auto timestamp = [&](uint32_t q, slag::CommandBuffer* commandBuffer, uint32_t which) {
        if (timed[q])
        {
            commandBuffer->writeTimestamp(pools[q], static_cast<uint32_t>(frame % framesInFlight) * 2 + which);
        }
    };
    auto resolve = [&](uint32_t q, slag::CommandBuffer* commandBuffer) {
        if (timed[q])
        {
            uint32_t first = static_cast<uint32_t>(frame % framesInFlight) * 2;
            commandBuffer->resolveQueries(pools[q], first, 2, results[q], first * sizeof(uint64_t));
        }
    };

    int result = app.run([&](slag::CommandBuffer* commandBuffer, slag::AttachmentView* swapchainView, uint32_t width,
                             uint32_t height) {
        uint32_t slot = static_cast<uint32_t>(frame % framesInFlight);
        uint32_t current = static_cast<uint32_t>(frame % 2);
        uint32_t previous = 1 - current;
        // The frame loop waited for frame - 2 on the graphics queue, which
        // includes its particle pass; wait for compute and copy frame - 2 too
        // before reusing their command buffers and results.
        if (frame >= framesInFlight)
        {
            slag::SemaphoreValue done[] = {{computeDone, frame - 1}, {copyDone, frame - 1}};
            device->waitSemaphores(done, slag::WaitMode::All, UINT64_MAX);
            for (uint32_t q = 0; q < QueueCount; ++q)
            {
                collect(q, frame - framesInFlight);
            }
        }
        // Compute and copy write what particle pass frame - 1 read, so they wait for it.
        slag::SemaphoreValue previousDraw = {particlesDrawn, frame};

        // Compute: buffer `previous` -> buffer `current`.
        slag::CommandBuffer* compute = computeLists[slot];
        compute->reset();
        compute->begin();
        timestamp(ComputeQueue, compute, 0);
        // Compute frame - 1 wrote the input on this queue, in an earlier submit.
        slag::GlobalBarrier afterUpdate = {.syncBefore = slag::BarrierSync::ComputeShading,
                                           .syncAfter = slag::BarrierSync::ComputeShading,
                                           .accessBefore = slag::BarrierAccess::ShaderWrite,
                                           .accessAfter = slag::BarrierAccess::ShaderRead};
        compute->barrier({&afterUpdate, 1});
        compute->bindPipeline(updatePipeline);
        struct
        {
            uint32_t input;
            uint32_t output;
            uint32_t count;
            float time;
        } updatePush = {ParticlesRead + previous, ParticlesWrite + current, particleCount,
                        static_cast<float>(frame) / 60.0f};
        compute->pushConstants(&updatePush, sizeof(updatePush));
        compute->dispatch(particleCount / 64, 1, 1);
        timestamp(ComputeQueue, compute, 1);
        resolve(ComputeQueue, compute);
        compute->end();
        slag::SemaphoreValue computeSignal = {computeDone, frame + 1};
        slag::CommandBuffer* computeList[] = {compute};
        queues[ComputeQueue]->submit(
            {.waits = {&previousDraw, 1}, .commandBuffers = computeList, .signals = {&computeSignal, 1}});

        // Copy: a staging content -> texture `current`, in Common.
        slag::CommandBuffer* copy = copyLists[slot];
        copy->reset();
        copy->begin();
        timestamp(CopyQueue, copy, 0);
        uint64_t content = (frame / framesPerContent) % stagingCount;
        copy->copyBufferToTexture({.texture = textures[current]},
                                  {staging, content * footprint.size, footprint.rowPitch}, textureSize, textureSize);
        timestamp(CopyQueue, copy, 1);
        resolve(CopyQueue, copy);
        copy->end();
        slag::SemaphoreValue copySignal = {copyDone, frame + 1};
        slag::CommandBuffer* copyList[] = {copy};
        queues[CopyQueue]->submit(
            {.waits = {&previousDraw, 1}, .commandBuffers = copyList, .signals = {&copySignal, 1}});

        // Particle pass, its own graphics submit: draws what compute and copy
        // made in frame - 1.
        slag::CommandBuffer* draw = drawLists[slot];
        draw->reset();
        draw->begin();
        timestamp(GraphicsQueue, draw, 0);
        slag::TextureBarrier before[] = {{.syncAfter = slag::BarrierSync::VertexShading,
                                          .accessAfter = slag::BarrierAccess::ShaderRead,
                                          .layoutBefore = slag::TextureLayout::Common,
                                          .layoutAfter = slag::TextureLayout::ShaderRead,
                                          .texture = textures[previous]},
                                         // The previous composite read the target; its contents are cleared.
                                         {.syncBefore = slag::BarrierSync::PixelShading,
                                          .syncAfter = slag::BarrierSync::RenderTarget,
                                          .accessAfter = slag::BarrierAccess::RenderTarget,
                                          .layoutBefore = slag::TextureLayout::Undefined,
                                          .layoutAfter = slag::TextureLayout::RenderTarget,
                                          .texture = target}};
        draw->barrier({}, {}, before);
        const slag::ColorAttachment targetColor = {
            .view = targetView, .loadOp = slag::LoadOp::Clear, .clearValue = {.color = {0.0f, 0.0f, 0.0f, 1.0f}}};
        draw->beginRendering({.colorAttachments = {&targetColor, 1}});
        if (frame > 0)
        {
            draw->bindPipeline(drawPipeline);
            struct
            {
                uint32_t particles;
                uint32_t texture;
                uint32_t sampler;
                float aspect;
                float size[2];
            } drawPush = {ParticlesRead + previous,
                          TextureSlots + previous,
                          samplerSlot,
                          1.0f,
                          {6.0f / static_cast<float>(targetSize), 6.0f / static_cast<float>(targetSize)}};
            draw->pushConstants(&drawPush, sizeof(drawPush));
            draw->draw(6, particleCount);
        }
        draw->endRendering();
        slag::TextureBarrier after[] = {{.syncBefore = slag::BarrierSync::VertexShading,
                                         .accessBefore = slag::BarrierAccess::ShaderRead,
                                         .layoutBefore = slag::TextureLayout::ShaderRead,
                                         .layoutAfter = slag::TextureLayout::Common,
                                         .texture = textures[previous]},
                                        {.syncBefore = slag::BarrierSync::RenderTarget,
                                         .syncAfter = slag::BarrierSync::PixelShading,
                                         .accessBefore = slag::BarrierAccess::RenderTarget,
                                         .accessAfter = slag::BarrierAccess::ShaderRead,
                                         .layoutBefore = slag::TextureLayout::RenderTarget,
                                         .layoutAfter = slag::TextureLayout::ShaderRead,
                                         .texture = target}};
        draw->barrier({}, {}, after);
        timestamp(GraphicsQueue, draw, 1);
        resolve(GraphicsQueue, draw);
        draw->end();
        slag::SemaphoreValue waits[] = {{computeDone, frame}, {copyDone, frame}};
        slag::SemaphoreValue drawSignal = {particlesDrawn, frame + 1};
        slag::CommandBuffer* drawList[] = {draw};
        queues[GraphicsQueue]->submit({.waits = waits, .commandBuffers = drawList, .signals = {&drawSignal, 1}});

        // The swapchain pass, submitted after the particle pass on the same queue.
        const slag::ColorAttachment color = {.view = swapchainView, .loadOp = slag::LoadOp::Discard};
        commandBuffer->beginRendering({.colorAttachments = {&color, 1}});
        commandBuffer->bindPipeline(compositePipeline);
        struct
        {
            uint32_t texture;
            uint32_t sampler;
            float aspect;
        } compositePush = {TargetSlot, samplerSlot, static_cast<float>(height) / static_cast<float>(width)};
        commandBuffer->pushConstants(&compositePush, sizeof(compositePush));
        commandBuffer->draw(3, 1);
        commandBuffer->endRendering();
        ++frame;
    });

    device->waitIdle();
    for (uint64_t f = frame >= framesInFlight ? frame - framesInFlight : 0; f < frame; ++f)
    {
        for (uint32_t q = 0; q < QueueCount; ++q)
        {
            collect(q, f);
        }
    }

    if (!calibrated)
    {
        std::printf("timestamp calibration isn't supported here, so overlap isn't measured\n");
    }
    else
    {
        std::vector<Interval>& graphics = intervals[GraphicsQueue];
        std::sort(graphics.begin(), graphics.end(),
                  [](const Interval& a, const Interval& b) { return a.start < b.start; });
        auto busy = [](const std::vector<Interval>& list) {
            int64_t total = 0;
            for (const Interval& interval : list)
            {
                total += interval.end - interval.start;
            }
            return total;
        };
        for (uint32_t q = 0; q < QueueCount; ++q)
        {
            if (!timed[q])
            {
                std::printf("%-8s  no timestamps on this queue type here\n", queueNames[q]);
                continue;
            }
            size_t count = intervals[q].size();
            double perFrame = count ? static_cast<double>(busy(intervals[q])) / 1e3 / static_cast<double>(count) : 0;
            if (q == GraphicsQueue)
            {
                std::printf("%-8s  %6.1f us per frame over %zu frames\n", queueNames[q], perFrame, count);
                continue;
            }
            int64_t total = busy(intervals[q]);
            double overlapped = total ? 100.0 * static_cast<double>(overlap(intervals[q], graphics)) / total : 0.0;
            std::printf("%-8s  %6.1f us per frame, %5.1f%% of it overlapped graphics work\n", queueNames[q], perFrame,
                        overlapped);
        }
    }

    for (uint32_t i = 0; i < framesInFlight; ++i)
    {
        device->destroy(computeLists[i]);
        device->destroy(copyLists[i]);
        device->destroy(drawLists[i]);
    }
    for (uint32_t q = 0; q < QueueCount; ++q)
    {
        device->destroy(pools[q]);
        device->destroy(results[q]);
    }
    for (uint32_t i = 0; i < 2; ++i)
    {
        device->destroy(particles[i]);
        device->destroy(textures[i]);
    }
    device->destroy(staging);
    device->destroy(copyDone);
    device->destroy(computeDone);
    device->destroy(particlesDrawn);
    device->destroy(targetView);
    device->destroy(target);
    device->destroy(compositePipeline);
    device->destroy(drawPipeline);
    device->destroy(updatePipeline);
    return result;
}
