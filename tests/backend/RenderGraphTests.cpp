// The render graph module (ADR 0072) on every backend, under synchronization
// validation: barriers and layout handoffs between queues, transients sharing
// memory in one submit, and frames recorded from several threads.

#include "TestDevice.h"

#include <slag/rendergraph/RenderGraph.h>

#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#include <thread>
#include <vector>

namespace {

std::vector<char> readFile(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

constexpr uint32_t size = 64;

struct GraphTest
{
    explicit GraphTest(slag::Backend backend)
        : test(backend, slag::ValidationFailureAction::Continue,
               slag::ValidationFlags::Api | slag::ValidationFlags::Synchronization)
    {
        device = test.firstDevice();
        REQUIRE(device);
        REQUIRE(static_cast<bool>(device->initialize({})));
    }

    ~GraphTest()
    {
        slag::rendergraph::destroy(graph);
        for (slag::Buffer* buffer : buffers)
        {
            device->destroy(buffer);
        }
        device->destroy(pipeline);
        device->shutdown();
    }

    slag::rendergraph::RenderGraph* createGraph(uint32_t framesInFlight = 2)
    {
        slag::Result<slag::rendergraph::RenderGraph*> created =
            slag::rendergraph::createRenderGraph({.device = device,
                                                  .framesInFlight = framesInFlight,
                                                  .messageCallback = collect,
                                                  .messageUserData = &graphErrors});
        REQUIRE(static_cast<bool>(created));
        graph = created.value();
        return graph;
    }

    static void collect(const slag::DebugMessage& debugMessage, void* userData)
    {
        const char* message = debugMessage.text;
        static_cast<std::vector<std::string>*>(userData)->push_back(message);
    }

    slag::Buffer* readback(uint64_t bytes)
    {
        slag::Buffer* buffer =
            device->createBuffer({.size = bytes, .placement = slag::Placement::automatic(slag::MemoryKind::Readback)})
                .value();
        REQUIRE(buffer);
        buffers.push_back(buffer);
        return buffer;
    }

    slag::Pipeline* fillPipeline()
    {
        std::vector<char> spirv = readFile(SLAG_SHADER_DIR "/FillBuffer.main.spv");
        std::vector<char> dxil = readFile(SLAG_SHADER_DIR "/FillBuffer.main.dxil");
        std::vector<slag::ShaderBinary> binaries;
        if (!dxil.empty())
        {
            binaries.push_back({slag::ShaderFormat::Dxil, dxil.data(), dxil.size()});
        }
        if (!spirv.empty())
        {
            binaries.push_back({slag::ShaderFormat::Spirv, spirv.data(), spirv.size()});
        }
        pipeline = device->createComputePipeline({.compute = {.binaries = binaries}}).value();
        REQUIRE(pipeline);
        return pipeline;
    }

    void finish()
    {
        for (const std::string& error : test.messages.errors)
        {
            MESSAGE("error: " << error);
        }
        REQUIRE(static_cast<bool>(device->waitIdle()));
        for (const std::string& error : graphErrors)
        {
            MESSAGE("render graph: " << error);
        }
        CHECK(graphErrors.empty());
        test.checkNoErrors();
    }

    TestInstance test;
    slag::GraphicsDevice* device = nullptr;
    slag::rendergraph::RenderGraph* graph = nullptr;
    slag::Pipeline* pipeline = nullptr;
    std::vector<slag::Buffer*> buffers;
    std::vector<std::string> graphErrors;
};

// Copies mip 0 of a texture into a readback buffer laid out as copyFootprint says.
void copyToBuffer(slag::GraphicsDevice* device, slag::CommandBuffer* commandBuffer, slag::Texture* texture,
                  slag::Buffer* buffer)
{
    slag::CopyFootprint footprint = device->copyFootprint(texture, 0);
    commandBuffer->copyTextureToBuffer({.buffer = buffer, .rowPitch = footprint.rowPitch}, {.texture = texture},
                                       footprint.width, footprint.height);
}

uint32_t countPixelsNot(slag::GraphicsDevice* device, slag::Buffer* buffer, uint32_t rowPitch, const uint8_t rgba[4])
{
    (void)device;
    const auto* bytes = static_cast<const uint8_t*>(buffer->mappedData());
    uint32_t wrong = 0;
    for (uint32_t y = 0; y < size; ++y)
    {
        for (uint32_t x = 0; x < size; ++x)
        {
            const uint8_t* pixel = bytes + y * rowPitch + x * 4;
            for (int c = 0; c < 4; ++c)
            {
                int difference = pixel[c] - rgba[c];
                wrong += difference > 1 || difference < -1 ? 1 : 0;
            }
        }
    }
    return wrong;
}

constexpr slag::Texture2DDesc targetDesc = {.format = slag::PixelFormat::R8G8B8A8Unorm,
                                            .width = size,
                                            .height = size,
                                            .usage = slag::TextureUsage::RenderTarget};

// A render target cleared on the graphics queue, copied out on the copy
// queue: the graph moves it to Common before the handoff.
void testHandoff(slag::Backend backend)
{
    GraphTest t(backend);
    slag::rendergraph::RenderGraph* graph = t.createGraph();
    slag::Buffer* readback = t.readback(256 * size);
    const slag::ClearValue clear = {.color = {0.25f, 0.5f, 0.75f, 1.0f}};

    REQUIRE(static_cast<bool>(graph->beginFrame()));
    slag::rendergraph::TextureHandle target = graph->createTransientTexture2D(targetDesc);
    slag::rendergraph::BufferHandle output = graph->importBuffer(readback);
    graph->addPass(
        "clear", slag::CommandQueueType::Graphics,
        [&](slag::rendergraph::PassBuilder& pass) {
            pass.renderTarget(target, slag::LoadOp::Clear, slag::StoreOp::Store, clear);
        },
        [](slag::rendergraph::PassContext&, slag::CommandBuffer*) {});
    slag::GraphicsDevice* device = t.device;
    graph->addPass(
        "copy out", slag::CommandQueueType::Copy,
        [&](slag::rendergraph::PassBuilder& pass) {
            pass.use(target, slag::rendergraph::Usage::CopySource);
            pass.use(output, slag::rendergraph::Usage::CopyDest);
        },
        [=](slag::rendergraph::PassContext& context, slag::CommandBuffer* commandBuffer) {
            copyToBuffer(device, commandBuffer, context.texture(target), context.buffer(output));
        });
    REQUIRE(static_cast<bool>(graph->compile()));
    for (uint32_t i = 0; i < graph->passCount(); ++i)
    {
        CHECK(static_cast<bool>(graph->recordPass(i)));
    }
    CHECK(static_cast<bool>(graph->execute()));
    t.finish();
    const uint8_t expected[4] = {64, 128, 191, 255};
    CHECK(countPixelsNot(t.device, readback, 256, expected) == 0);
}

// Two render targets with ordered uses share memory within one submit; both
// copies still see their own clear color.
void testAliasing(slag::Backend backend)
{
    GraphTest t(backend);
    slag::rendergraph::RenderGraph* graph = t.createGraph();
    slag::Buffer* first = t.readback(256 * size);
    slag::Buffer* second = t.readback(256 * size);
    slag::GraphicsDevice* device = t.device;

    REQUIRE(static_cast<bool>(graph->beginFrame()));
    slag::rendergraph::TextureHandle a = graph->createTransientTexture2D(targetDesc);
    slag::rendergraph::TextureHandle b = graph->createTransientTexture2D(targetDesc);
    slag::rendergraph::BufferHandle outA = graph->importBuffer(first);
    slag::rendergraph::BufferHandle outB = graph->importBuffer(second);
    auto clearPass = [&](const char* name, slag::rendergraph::TextureHandle target, const slag::ClearValue& clear) {
        graph->addPass(
            name, slag::CommandQueueType::Graphics,
            [&](slag::rendergraph::PassBuilder& pass) {
                pass.renderTarget(target, slag::LoadOp::Clear, slag::StoreOp::Store, clear);
            },
            [](slag::rendergraph::PassContext&, slag::CommandBuffer*) {});
    };
    auto copyPass = [&](const char* name, slag::rendergraph::TextureHandle source,
                        slag::rendergraph::BufferHandle destination) {
        graph->addPass(
            name, slag::CommandQueueType::Graphics,
            [&](slag::rendergraph::PassBuilder& pass) {
                pass.use(source, slag::rendergraph::Usage::CopySource);
                pass.use(destination, slag::rendergraph::Usage::CopyDest);
            },
            [=](slag::rendergraph::PassContext& context, slag::CommandBuffer* commandBuffer) {
                copyToBuffer(device, commandBuffer, context.texture(source), context.buffer(destination));
            });
    };
    clearPass("clear a", a, {.color = {1.0f, 0.0f, 0.0f, 1.0f}});
    copyPass("copy a", a, outA);
    clearPass("clear b", b, {.color = {0.0f, 1.0f, 0.0f, 1.0f}});
    copyPass("copy b", b, outB);
    REQUIRE(static_cast<bool>(graph->compile()));
    slag::PlacementInfo info = t.device->placementInfo(targetDesc);
    CHECK(graph->transientMemorySize() == info.size); // one texture's worth
    for (uint32_t i = 0; i < graph->passCount(); ++i)
    {
        CHECK(static_cast<bool>(graph->recordPass(i)));
    }
    CHECK(static_cast<bool>(graph->execute()));
    t.finish();
    const uint8_t red[4] = {255, 0, 0, 255};
    const uint8_t green[4] = {0, 255, 0, 255};
    CHECK(countPixelsNot(t.device, first, 256, red) == 0);
    CHECK(countPixelsNot(t.device, second, 256, green) == 0);
}

// Compute fills a transient buffer, graphics copies it to another, the copy
// queue reads that back: three submits chained by semaphores. Four frames
// with two in flight, every pass recorded on its own thread.
void testQueuesAndThreads(slag::Backend backend)
{
    GraphTest t(backend);
    slag::rendergraph::RenderGraph* graph = t.createGraph(2);
    slag::Pipeline* pipeline = t.fillPipeline();
    constexpr uint32_t count = 4096;
    slag::Buffer* readbacks[2] = {t.readback(count * 4), t.readback(count * 4)};
    slag::GraphicsDevice* device = t.device;
    uint64_t firstSize = 0;
    for (uint32_t frame = 0; frame < 4; ++frame)
    {
        REQUIRE(static_cast<bool>(graph->beginFrame()));
        slag::rendergraph::BufferHandle filled = graph->createTransientBuffer({.size = count * 4});
        slag::rendergraph::BufferHandle copied = graph->createTransientBuffer({.size = count * 4});
        slag::rendergraph::BufferHandle output = graph->importBuffer(readbacks[frame % 2]);
        graph->addPass(
            "fill", slag::CommandQueueType::Compute,
            [&](slag::rendergraph::PassBuilder& pass) { pass.use(filled, slag::rendergraph::Usage::ShaderWrite); },
            [=](slag::rendergraph::PassContext& context, slag::CommandBuffer* commandBuffer) {
                uint32_t slot = 20 + context.frameSlot(); // per frame in flight
                device->writeRWStructuredBuffer(slot, {.buffer = context.buffer(filled), .count = count, .stride = 4});
                commandBuffer->bindPipeline(pipeline);
                const uint32_t push[] = {slot, count};
                commandBuffer->pushConstants(push, sizeof(push));
                commandBuffer->dispatch(count / 64, 1, 1);
            });
        graph->addPass(
            "copy", slag::CommandQueueType::Graphics,
            [&](slag::rendergraph::PassBuilder& pass) {
                pass.use(filled, slag::rendergraph::Usage::CopySource);
                pass.use(copied, slag::rendergraph::Usage::CopyDest);
            },
            [=](slag::rendergraph::PassContext& context, slag::CommandBuffer* commandBuffer) {
                commandBuffer->copyBuffer(context.buffer(copied), 0, context.buffer(filled), 0, count * 4);
            });
        graph->addPass(
            "read back", slag::CommandQueueType::Copy,
            [&](slag::rendergraph::PassBuilder& pass) {
                pass.use(copied, slag::rendergraph::Usage::CopySource);
                pass.use(output, slag::rendergraph::Usage::CopyDest);
            },
            [=](slag::rendergraph::PassContext& context, slag::CommandBuffer* commandBuffer) {
                commandBuffer->copyBuffer(context.buffer(output), 0, context.buffer(copied), 0, count * 4);
            });
        REQUIRE(static_cast<bool>(graph->compile()));
        firstSize = frame == 0 ? graph->transientMemorySize() : firstSize;
        CHECK(graph->transientMemorySize() == firstSize);
        std::vector<std::thread> threads;
        std::vector<int> recorded(graph->passCount(), 0);
        for (uint32_t i = 0; i < graph->passCount(); ++i)
        {
            threads.emplace_back([&, i] { recorded[i] = static_cast<bool>(graph->recordPass(i)) ? 1 : 0; });
        }
        for (std::thread& thread : threads)
        {
            thread.join();
        }
        for (int ok : recorded)
        {
            REQUIRE(ok == 1);
        }
        CHECK(static_cast<bool>(graph->execute()));
    }
    t.finish();
    for (slag::Buffer* readback : readbacks)
    {
        const auto* values = static_cast<const uint32_t*>(readback->mappedData());
        uint32_t wrong = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            wrong += values[i] == i * 2 + 1 ? 0 : 1;
        }
        CHECK(wrong == 0);
    }
}

// A defragmentation pass run by the graph (ADR 0062, 0072): movable buffers
// and textures, three quarters freed. One frame uses a texture and leaves it
// in ShaderRead; the rest rest in Common, which layoutOf reports. Afterwards
// every buffer and texture still holds its stamp.
void testDefragmentation(slag::Backend backend)
{
    GraphTest t(backend);
    slag::GraphicsDevice* device = t.device;
    slag::rendergraph::RenderGraph* graph = t.createGraph();
    constexpr uint32_t bufferCount = 1024;
    constexpr uint64_t bufferSize = 1 << 20;
    constexpr uint32_t stamp = 256;
    constexpr uint32_t side = 512; // 1 MB textures spread across blocks like the buffers
    constexpr uint32_t texels = side * side * 4;
    auto byteOf = [](uint32_t id, uint32_t i) {
        return static_cast<uint8_t>(id * 31 + i * 7 + 1);
    };
    std::vector<slag::Buffer*> buffers;
    std::vector<slag::Texture*> textures;
    for (uint32_t i = 0; i < bufferCount; ++i)
    {
        buffers.push_back(
            device
                ->createBuffer({.size = bufferSize,
                                .placement = slag::Placement::automatic(slag::MemoryKind::GpuOnly, {.movable = true})})
                .value());
        if (i % 32 == 0)
        {
            textures.push_back(device
                                   ->createTexture2D({.format = slag::PixelFormat::R8G8B8A8Unorm,
                                                      .width = side,
                                                      .height = side,
                                                      .placement = slag::Placement::automatic(slag::MemoryKind::GpuOnly,
                                                                                              {.movable = true})})
                                   .value());
        }
    }
    slag::Buffer* staging = device
                                ->createBuffer({.size = uint64_t(bufferCount) * stamp + textures.size() * texels,
                                                .placement = slag::Placement::automatic(slag::MemoryKind::Upload)})
                                .value();
    REQUIRE(staging);
    t.buffers.push_back(staging);
    auto* bytes = static_cast<uint8_t*>(staging->mappedData());
    for (uint32_t id = 0; id < bufferCount; ++id)
    {
        for (uint32_t i = 0; i < stamp; ++i)
        {
            bytes[uint64_t(id) * stamp + i] = byteOf(id, i);
        }
    }
    const uint64_t textureStaging = uint64_t(bufferCount) * stamp;
    for (size_t k = 0; k < textures.size(); ++k)
    {
        for (uint32_t i = 0; i < texels; ++i)
        {
            bytes[textureStaging + k * texels + i] = byteOf(10'000 + static_cast<uint32_t>(k), i);
        }
    }

    // Frame 0: upload every stamp; textures end in Common.
    REQUIRE(static_cast<bool>(graph->beginFrame()));
    {
        slag::rendergraph::BufferHandle source = graph->importBuffer(staging);
        std::vector<slag::rendergraph::BufferHandle> bufferHandles;
        std::vector<slag::rendergraph::TextureHandle> textureHandles;
        for (slag::Buffer* buffer : buffers)
        {
            bufferHandles.push_back(graph->importBuffer(buffer));
        }
        for (slag::Texture* texture : textures)
        {
            textureHandles.push_back(graph->importTexture(texture));
        }
        graph->addPass(
            "upload", slag::CommandQueueType::Graphics,
            [&](slag::rendergraph::PassBuilder& pass) {
                pass.use(source, slag::rendergraph::Usage::CopySource);
                for (auto handle : bufferHandles)
                {
                    pass.use(handle, slag::rendergraph::Usage::CopyDest);
                }
                for (auto handle : textureHandles)
                {
                    pass.use(handle, slag::rendergraph::Usage::CopyDest);
                }
            },
            [=](slag::rendergraph::PassContext&, slag::CommandBuffer* commandBuffer) {
                for (uint32_t id = 0; id < bufferCount; ++id)
                {
                    commandBuffer->copyBuffer(buffers[id], 0, staging, uint64_t(id) * stamp, stamp);
                }
                for (size_t k = 0; k < textures.size(); ++k)
                {
                    commandBuffer->copyBufferToTexture({.texture = textures[k]},
                                                       {staging, textureStaging + k * texels, side * 4}, side, side);
                }
            });
        REQUIRE(static_cast<bool>(graph->compile()));
        CHECK(static_cast<bool>(graph->recordPass(0)));
        CHECK(static_cast<bool>(graph->execute()));
    }

    // Keep a quarter of the buffers and half the textures.
    std::vector<uint32_t> keptBuffers;
    std::vector<uint32_t> keptTextures;
    REQUIRE(static_cast<bool>(device->waitIdle()));
    for (uint32_t id = 0; id < bufferCount; ++id)
    {
        if (id % 4 == 0)
        {
            keptBuffers.push_back(id);
        }
        else
        {
            device->destroy(buffers[id]);
        }
    }
    for (uint32_t k = 0; k < textures.size(); ++k)
    {
        if (k % 2 == 0)
        {
            keptTextures.push_back(k);
        }
        else
        {
            device->destroy(textures[k]);
        }
    }

    // Frames with a pass each, until nothing moves. Half the kept textures
    // are used and left in ShaderRead (the graph knows their layout); the
    // rest rest in Common, which layoutOf reports.
    auto imported = [&](slag::Texture* texture) {
        for (size_t i = 0; i < keptTextures.size(); i += 2)
        {
            if (textures[keptTextures[i]] == texture)
            {
                return true;
            }
        }
        return false;
    };
    size_t movedImported = 0;
    size_t movedOthers = 0;
    size_t movedTotal = 0;
    for (uint32_t frame = 0;; ++frame)
    {
        REQUIRE(frame < 32);
        REQUIRE(static_cast<bool>(graph->beginFrame()));
        std::vector<slag::rendergraph::TextureHandle> handles;
        for (size_t i = 0; i < keptTextures.size(); i += 2)
        {
            handles.push_back(graph->importTexture(
                textures[keptTextures[i]],
                {.initialLayout = frame == 0 ? slag::TextureLayout::Common : slag::TextureLayout::ShaderRead,
                 .finalLayout = slag::TextureLayout::ShaderRead}));
        }
        graph->addPass(
            "sample", slag::CommandQueueType::Graphics,
            [&](slag::rendergraph::PassBuilder& pass) {
                for (auto handle : handles)
                {
                    pass.use(handle, slag::rendergraph::Usage::ShaderRead);
                }
            },
            [](slag::rendergraph::PassContext&, slag::CommandBuffer*) {});
        uint32_t asked = 0;
        graph->defragment({}, [&](slag::Texture* texture) {
            ++asked;
            CHECK_FALSE(imported(texture)); // the graph knows those
            return slag::TextureLayout::Common;
        });
        REQUIRE(static_cast<bool>(graph->compile()));
        CHECK(static_cast<bool>(graph->recordPass(0)));
        CHECK(static_cast<bool>(graph->execute()));
        size_t others = 0;
        for (const slag::DefragmentationMove& move : graph->movedResources())
        {
            movedImported += move.texture && imported(move.texture) ? 1 : 0;
            others += move.texture && !imported(move.texture) ? 1 : 0;
        }
        CHECK(asked == others);
        movedOthers += others;
        movedTotal += graph->movedResources().size();
        if (graph->movedResources().empty())
        {
            break;
        }
    }
    MESSAGE(movedTotal << " moves; textures: " << movedImported << " imported, " << movedOthers << " not");
    CHECK(movedTotal > 0);
    // Where textures land depends on the device's block size: on RADV and
    // NVIDIA both groups move; on WARP (small heaps) none may.
    WARN(movedImported + movedOthers > 0);

    // Frame 2: read every stamp back.
    slag::Buffer* readback = t.readback(keptBuffers.size() * stamp + keptTextures.size() * texels);
    REQUIRE(static_cast<bool>(graph->beginFrame()));
    {
        slag::rendergraph::BufferHandle output = graph->importBuffer(readback);
        std::vector<slag::rendergraph::BufferHandle> bufferHandles;
        std::vector<slag::rendergraph::TextureHandle> textureHandles;
        for (uint32_t id : keptBuffers)
        {
            bufferHandles.push_back(graph->importBuffer(buffers[id]));
        }
        for (uint32_t k : keptTextures)
        {
            slag::TextureLayout initial =
                imported(textures[k]) ? slag::TextureLayout::ShaderRead : slag::TextureLayout::Common;
            textureHandles.push_back(graph->importTexture(textures[k], {.initialLayout = initial}));
        }
        graph->addPass(
            "read back", slag::CommandQueueType::Graphics,
            [&](slag::rendergraph::PassBuilder& pass) {
                pass.use(output, slag::rendergraph::Usage::CopyDest);
                for (auto handle : bufferHandles)
                {
                    pass.use(handle, slag::rendergraph::Usage::CopySource);
                }
                for (auto handle : textureHandles)
                {
                    pass.use(handle, slag::rendergraph::Usage::CopySource);
                }
            },
            [=](slag::rendergraph::PassContext&, slag::CommandBuffer* commandBuffer) {
                for (size_t i = 0; i < keptBuffers.size(); ++i)
                {
                    commandBuffer->copyBuffer(readback, i * stamp, buffers[keptBuffers[i]], 0, stamp);
                }
                uint64_t base = keptBuffers.size() * stamp;
                for (size_t i = 0; i < keptTextures.size(); ++i)
                {
                    commandBuffer->copyTextureToBuffer({readback, base + i * texels, side * 4},
                                                       {.texture = textures[keptTextures[i]]}, side, side);
                }
            });
        REQUIRE(static_cast<bool>(graph->compile()));
        CHECK(static_cast<bool>(graph->recordPass(0)));
        CHECK(static_cast<bool>(graph->execute()));
    }
    t.finish();
    const auto* result = static_cast<const uint8_t*>(readback->mappedData());
    uint32_t bad = 0;
    for (size_t i = 0; i < keptBuffers.size(); ++i)
    {
        for (uint32_t b = 0; b < stamp; ++b)
        {
            bad += result[i * stamp + b] == byteOf(keptBuffers[i], b) ? 0 : 1;
        }
    }
    uint64_t base = keptBuffers.size() * stamp;
    for (size_t i = 0; i < keptTextures.size(); ++i)
    {
        for (uint32_t b = 0; b < texels; ++b)
        {
            bad += result[base + i * texels + b] == byteOf(10'000 + keptTextures[i], b) ? 0 : 1;
        }
    }
    CHECK(bad == 0);
    for (uint32_t id : keptBuffers)
    {
        device->destroy(buffers[id]);
    }
    for (uint32_t k : keptTextures)
    {
        device->destroy(textures[k]);
    }
}

// Per-subresource state and every transient texture type: each mip of a
// chain (each pass reading the mip before), each cube face, three array
// layers in one pass, and two 3D slices are cleared to their own color, then
// copied out and checked.
void testSubresources(slag::Backend backend)
{
    GraphTest t(backend);
    slag::rendergraph::RenderGraph* graph = t.createGraph();
    slag::GraphicsDevice* device = t.device;
    constexpr uint64_t slot = 64 * 1024; // readback bytes per image
    constexpr uint32_t images = 4 + 6 + 3 + 1;
    slag::Buffer* readback = t.readback(slot * images);
    auto colorOf = [](uint32_t i) {
        float v = static_cast<float>(i * 16 + 8) / 255.0f;
        return slag::ClearValue{.color = {v, 1.0f - v, v * 0.5f, 1.0f}};
    };
    auto rgba = colorOf; // the bytes each image should hold, from the same values
    REQUIRE(static_cast<bool>(graph->beginFrame()));
    constexpr slag::TextureUsage target = slag::TextureUsage::RenderTarget;
    constexpr slag::PixelFormat format = slag::PixelFormat::R8G8B8A8Unorm;
    slag::rendergraph::TextureHandle chain =
        graph->createTransientTexture2D({.format = format, .width = 64, .height = 64, .mipCount = 4, .usage = target});
    slag::rendergraph::TextureHandle cube =
        graph->createTransientTextureCube({.format = format, .size = 16, .usage = target});
    slag::rendergraph::TextureHandle array = graph->createTransientTexture2DArray(
        {.format = format, .width = 16, .height = 16, .layerCount = 3, .usage = target});
    slag::rendergraph::TextureHandle volume =
        graph->createTransientTexture3D({.format = format, .width = 16, .height = 16, .depth = 2, .usage = target});
    slag::rendergraph::BufferHandle output = graph->importBuffer(readback);
    auto nothing = [](slag::rendergraph::PassContext&, slag::CommandBuffer*) {
    };
    uint32_t image = 0;
    for (uint32_t mip = 0; mip < 4; ++mip, ++image)
    {
        graph->addPass(
            "mip", slag::CommandQueueType::Graphics,
            [&](slag::rendergraph::PassBuilder& pass) {
                if (mip > 0)
                {
                    pass.use(chain, slag::rendergraph::Usage::ShaderRead, {.baseMip = mip - 1, .mipCount = 1});
                }
                pass.renderTarget(chain, slag::LoadOp::Clear, slag::StoreOp::Store, colorOf(image), mip);
            },
            nothing);
    }
    for (uint32_t face = 0; face < 6; ++face, ++image)
    {
        graph->addPass(
            "face", slag::CommandQueueType::Graphics,
            [&](slag::rendergraph::PassBuilder& pass) {
                pass.renderTarget(cube, slag::LoadOp::Clear, slag::StoreOp::Store, colorOf(image), 0, face);
            },
            nothing);
    }
    graph->addPass(
        "layers", slag::CommandQueueType::Graphics,
        [&](slag::rendergraph::PassBuilder& pass) {
            for (uint32_t layer = 0; layer < 3; ++layer)
            {
                pass.renderTarget(array, slag::LoadOp::Clear, slag::StoreOp::Store, colorOf(image + layer), 0, layer);
            }
        },
        nothing);
    image += 3;
    for (uint32_t slice = 0; slice < 2; ++slice)
    {
        graph->addPass(
            "slice", slag::CommandQueueType::Graphics,
            [&](slag::rendergraph::PassBuilder& pass) {
                // Both slices clear to one color, each through its own view.
                pass.renderTarget(volume, slag::LoadOp::Clear, slag::StoreOp::Store, colorOf(image), 0, slice);
            },
            nothing);
    }
    std::vector<uint32_t> pitches(images, 0);
    graph->addPass(
        "copy out", slag::CommandQueueType::Graphics,
        [&](slag::rendergraph::PassBuilder& pass) {
            for (auto texture : {chain, cube, array, volume})
            {
                pass.use(texture, slag::rendergraph::Usage::CopySource);
            }
            pass.use(output, slag::rendergraph::Usage::CopyDest);
        },
        [&](slag::rendergraph::PassContext& context, slag::CommandBuffer* commandBuffer) {
            uint32_t at = 0;
            auto copy = [&](slag::Texture* texture, uint32_t mip, uint32_t layer) {
                slag::CopyFootprint footprint = device->copyFootprint(texture, mip);
                pitches[at] = footprint.rowPitch;
                commandBuffer->copyTextureToBuffer(
                    {.buffer = context.buffer(output), .offset = at * slot, .rowPitch = footprint.rowPitch},
                    {.texture = texture, .mip = mip, .layer = layer}, footprint.width, footprint.height,
                    footprint.depth);
                ++at;
            };
            for (uint32_t mip = 0; mip < 4; ++mip)
            {
                copy(context.texture(chain), mip, 0);
            }
            for (uint32_t face = 0; face < 6; ++face)
            {
                copy(context.texture(cube), 0, face);
            }
            for (uint32_t layer = 0; layer < 3; ++layer)
            {
                copy(context.texture(array), 0, layer);
            }
            copy(context.texture(volume), 0, 0);
        });
    REQUIRE(static_cast<bool>(graph->compile()));
    for (uint32_t i = 0; i < graph->passCount(); ++i)
    {
        CHECK(static_cast<bool>(graph->recordPass(i)));
    }
    CHECK(static_cast<bool>(graph->execute()));
    t.finish();

    const auto* bytes = static_cast<const uint8_t*>(readback->mappedData());
    const uint32_t sizes[images] = {64, 32, 16, 8, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16};
    uint32_t wrong = 0;
    for (uint32_t i = 0; i < images; ++i)
    {
        slag::ClearValue color = rgba(i);
        uint8_t expected[4];
        for (int c = 0; c < 4; ++c)
        {
            expected[c] = static_cast<uint8_t>(color.color[c] * 255.0f + 0.5f);
        }
        uint32_t rows = sizes[i] * (i == images - 1 ? 2 : 1); // the volume's two slices
        for (uint32_t row = 0; row < rows; ++row)
        {
            for (uint32_t x = 0; x < sizes[i]; ++x)
            {
                const uint8_t* pixel = bytes + i * slot + uint64_t(row) * pitches[i] + x * 4;
                for (int c = 0; c < 4; ++c)
                {
                    int difference = pixel[c] - expected[c];
                    wrong += difference > 1 || difference < -1 ? 1 : 0;
                }
            }
        }
        CHECK_MESSAGE(wrong == 0, "image " << i);
    }
}

// Usage errors are reported and fail compile.
void testErrors(slag::Backend backend)
{
    GraphTest t(backend);
    slag::rendergraph::RenderGraph* graph = t.createGraph();
    REQUIRE(static_cast<bool>(graph->beginFrame()));
    slag::rendergraph::TextureHandle target = graph->createTransientTexture2D(targetDesc);
    graph->addPass(
        "reads garbage", slag::CommandQueueType::Graphics,
        [&](slag::rendergraph::PassBuilder& pass) { pass.use(target, slag::rendergraph::Usage::ShaderRead); },
        [](slag::rendergraph::PassContext&, slag::CommandBuffer*) {});
    CHECK(graph->compile().status() == slag::Status::Unsupported);
    REQUIRE(t.graphErrors.size() == 1);
    CHECK(t.graphErrors[0].find("read before anything writes it") != std::string::npos);
    t.graphErrors.clear();

    REQUIRE(static_cast<bool>(graph->beginFrame()));
    auto common = [](slag::Texture*) {
        return slag::TextureLayout::Common;
    };
    graph->defragment({}, common);
    graph->defragment({}, common);
    REQUIRE(t.graphErrors.size() == 1);
    CHECK(t.graphErrors[0].find("one defragmentation pass per frame") != std::string::npos);
    CHECK(graph->compile().status() == slag::Status::Unsupported); // a usage error fails the frame
    t.graphErrors.clear();
    REQUIRE(static_cast<bool>(graph->beginFrame()));
    graph->defragment({}, common);
    REQUIRE(static_cast<bool>(graph->compile()));
    CHECK(static_cast<bool>(graph->execute())); // nothing movable: an empty pass
    CHECK(graph->movedResources().empty());
    t.finish();
}

} // namespace

#ifdef SLAG_BACKEND_VULKAN
TEST_CASE("Vulkan: render graph - a render target handed to the copy queue")
{
    testHandoff(slag::Backend::Vulkan);
}
TEST_CASE("Vulkan: render graph - transients share memory in one submit")
{
    testAliasing(slag::Backend::Vulkan);
}
TEST_CASE("Vulkan: render graph - three queues, frames in flight, recorded on threads")
{
    testQueuesAndThreads(slag::Backend::Vulkan);
}
TEST_CASE("Vulkan: render graph - a defragmentation pass after the frame")
{
    testDefragmentation(slag::Backend::Vulkan);
}
TEST_CASE("Vulkan: render graph - mips, faces, layers, and slices")
{
    testSubresources(slag::Backend::Vulkan);
}
TEST_CASE("Vulkan: render graph - usage errors fail compile")
{
    testErrors(slag::Backend::Vulkan);
}
#endif

#ifdef SLAG_BACKEND_D3D12
TEST_CASE("D3D12: render graph - a render target handed to the copy queue")
{
    testHandoff(slag::Backend::D3D12);
}
TEST_CASE("D3D12: render graph - transients share memory in one submit")
{
    testAliasing(slag::Backend::D3D12);
}
TEST_CASE("D3D12: render graph - three queues, frames in flight, recorded on threads")
{
    testQueuesAndThreads(slag::Backend::D3D12);
}
TEST_CASE("D3D12: render graph - a defragmentation pass after the frame")
{
    testDefragmentation(slag::Backend::D3D12);
}
TEST_CASE("D3D12: render graph - mips, faces, layers, and slices")
{
    testSubresources(slag::Backend::D3D12);
}
TEST_CASE("D3D12: render graph - usage errors fail compile")
{
    testErrors(slag::Backend::D3D12);
}
#endif
