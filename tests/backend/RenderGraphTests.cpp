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
        slag::rg::destroy(graph);
        for (slag::Buffer* buffer : buffers)
        {
            device->destroy(buffer);
        }
        device->destroy(pipeline);
        device->shutdown();
    }

    slag::rg::RenderGraph* createGraph(uint32_t framesInFlight = 2)
    {
        slag::Result<slag::rg::RenderGraph*> created = slag::rg::createRenderGraph({.device = device,
                                                                                    .framesInFlight = framesInFlight,
                                                                                    .messageCallback = collect,
                                                                                    .messageUserData = &graphErrors});
        REQUIRE(static_cast<bool>(created));
        graph = created.value();
        return graph;
    }

    static void collect(slag::MessageSeverity, slag::MessageSource, const char* message, void* userData)
    {
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
    slag::rg::RenderGraph* graph = nullptr;
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
    slag::rg::RenderGraph* graph = t.createGraph();
    slag::Buffer* readback = t.readback(256 * size);
    const slag::ClearValue clear = {.color = {0.25f, 0.5f, 0.75f, 1.0f}};

    REQUIRE(static_cast<bool>(graph->beginFrame()));
    slag::rg::TextureRef target = graph->createTexture(targetDesc);
    slag::rg::BufferRef output = graph->importBuffer(readback);
    graph->addPass(
        "clear", slag::CommandQueueType::Graphics,
        [&](slag::rg::PassBuilder& pass) {
            pass.renderTarget(target, slag::LoadOp::Clear, slag::StoreOp::Store, clear);
        },
        [](slag::rg::PassContext&, slag::CommandBuffer*) {});
    slag::GraphicsDevice* device = t.device;
    graph->addPass(
        "copy out", slag::CommandQueueType::Copy,
        [&](slag::rg::PassBuilder& pass) {
            pass.use(target, slag::rg::Access::CopySource);
            pass.use(output, slag::rg::Access::CopyDest);
        },
        [=](slag::rg::PassContext& context, slag::CommandBuffer* commandBuffer) {
            copyToBuffer(device, commandBuffer, context.texture(target), context.buffer(output));
        });
    REQUIRE(static_cast<bool>(graph->compile()));
    for (uint32_t i = 0; i < graph->passCount(); ++i)
    {
        CHECK(static_cast<bool>(graph->recordPass(i)));
    }
    CHECK(static_cast<bool>(graph->submit()));
    t.finish();
    const uint8_t expected[4] = {64, 128, 191, 255};
    CHECK(countPixelsNot(t.device, readback, 256, expected) == 0);
}

// Two render targets with ordered uses share memory within one submit; both
// copies still see their own clear color.
void testAliasing(slag::Backend backend)
{
    GraphTest t(backend);
    slag::rg::RenderGraph* graph = t.createGraph();
    slag::Buffer* first = t.readback(256 * size);
    slag::Buffer* second = t.readback(256 * size);
    slag::GraphicsDevice* device = t.device;

    REQUIRE(static_cast<bool>(graph->beginFrame()));
    slag::rg::TextureRef a = graph->createTexture(targetDesc);
    slag::rg::TextureRef b = graph->createTexture(targetDesc);
    slag::rg::BufferRef outA = graph->importBuffer(first);
    slag::rg::BufferRef outB = graph->importBuffer(second);
    auto clearPass = [&](const char* name, slag::rg::TextureRef target, const slag::ClearValue& clear) {
        graph->addPass(
            name, slag::CommandQueueType::Graphics,
            [&](slag::rg::PassBuilder& pass) {
                pass.renderTarget(target, slag::LoadOp::Clear, slag::StoreOp::Store, clear);
            },
            [](slag::rg::PassContext&, slag::CommandBuffer*) {});
    };
    auto copyPass = [&](const char* name, slag::rg::TextureRef source, slag::rg::BufferRef destination) {
        graph->addPass(
            name, slag::CommandQueueType::Graphics,
            [&](slag::rg::PassBuilder& pass) {
                pass.use(source, slag::rg::Access::CopySource);
                pass.use(destination, slag::rg::Access::CopyDest);
            },
            [=](slag::rg::PassContext& context, slag::CommandBuffer* commandBuffer) {
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
    CHECK(static_cast<bool>(graph->submit()));
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
    slag::rg::RenderGraph* graph = t.createGraph(2);
    slag::Pipeline* pipeline = t.fillPipeline();
    constexpr uint32_t count = 4096;
    slag::Buffer* readbacks[2] = {t.readback(count * 4), t.readback(count * 4)};
    slag::GraphicsDevice* device = t.device;
    uint64_t firstSize = 0;
    for (uint32_t frame = 0; frame < 4; ++frame)
    {
        REQUIRE(static_cast<bool>(graph->beginFrame()));
        slag::rg::BufferRef filled = graph->createBuffer({.size = count * 4});
        slag::rg::BufferRef copied = graph->createBuffer({.size = count * 4});
        slag::rg::BufferRef output = graph->importBuffer(readbacks[frame % 2]);
        graph->addPass(
            "fill", slag::CommandQueueType::Compute,
            [&](slag::rg::PassBuilder& pass) { pass.use(filled, slag::rg::Access::ShaderWrite); },
            [=](slag::rg::PassContext& context, slag::CommandBuffer* commandBuffer) {
                uint32_t slot = 20 + context.frameSlot(); // per frame in flight
                device->writeRWStructuredBuffer(slot, {.buffer = context.buffer(filled), .count = count, .stride = 4});
                commandBuffer->bindPipeline(pipeline);
                const uint32_t push[] = {slot, count};
                commandBuffer->pushConstants(push, sizeof(push));
                commandBuffer->dispatch(count / 64, 1, 1);
            });
        graph->addPass(
            "copy", slag::CommandQueueType::Graphics,
            [&](slag::rg::PassBuilder& pass) {
                pass.use(filled, slag::rg::Access::CopySource);
                pass.use(copied, slag::rg::Access::CopyDest);
            },
            [=](slag::rg::PassContext& context, slag::CommandBuffer* commandBuffer) {
                commandBuffer->copyBuffer(context.buffer(copied), 0, context.buffer(filled), 0, count * 4);
            });
        graph->addPass(
            "read back", slag::CommandQueueType::Copy,
            [&](slag::rg::PassBuilder& pass) {
                pass.use(copied, slag::rg::Access::CopySource);
                pass.use(output, slag::rg::Access::CopyDest);
            },
            [=](slag::rg::PassContext& context, slag::CommandBuffer* commandBuffer) {
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
        CHECK(static_cast<bool>(graph->submit()));
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

// Usage errors are reported and fail compile.
void testErrors(slag::Backend backend)
{
    GraphTest t(backend);
    slag::rg::RenderGraph* graph = t.createGraph();
    REQUIRE(static_cast<bool>(graph->beginFrame()));
    slag::rg::TextureRef target = graph->createTexture(targetDesc);
    graph->addPass(
        "reads garbage", slag::CommandQueueType::Graphics,
        [&](slag::rg::PassBuilder& pass) { pass.use(target, slag::rg::Access::ShaderRead); },
        [](slag::rg::PassContext&, slag::CommandBuffer*) {});
    CHECK(graph->compile().status() == slag::Status::Unsupported);
    REQUIRE(t.graphErrors.size() == 1);
    CHECK(t.graphErrors[0].find("read before anything writes it") != std::string::npos);
    t.graphErrors.clear();
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
TEST_CASE("D3D12: render graph - usage errors fail compile")
{
    testErrors(slag::Backend::D3D12);
}
#endif
