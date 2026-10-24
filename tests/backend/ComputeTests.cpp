#include "TestDevice.h"

#include <slag/Buffer.h>
#include <slag/CommandBuffer.h>
#include <slag/GraphicsDevice.h>
#include <slag/Pipeline.h>

#include <cstddef>
#include <cstdint>
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

// The M1 spike (docs/milestones.md): FillBuffer.slang writes id * 2 + 1 into a
// RWStructuredBuffer reached through the resource heap; the CPU reads it back.
void runFillBuffer(TestInstance& test, slag::GraphicsDevice* device, bool indirect)
{
    MESSAGE("device: " << std::string(device->info().name));
    REQUIRE(static_cast<bool>(device->initialize({})));

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
    REQUIRE_FALSE(binaries.empty());

    slag::Result<slag::Pipeline*> pipeline = device->createComputePipeline(
        {.compute = {.binaries = binaries, .entryPoint = "main"}, .debugName = "FillBuffer"});
    REQUIRE(static_cast<bool>(pipeline));

    constexpr uint32_t elementCount = 10000;
    constexpr uint32_t outputSlot = 5;
    slag::Buffer* output = device
                               ->createBuffer({.size = elementCount * 4,
                                               .placement = slag::Placement::automatic(slag::MemoryKind::GpuOnly),
                                               .debugName = "output"})
                               .value();
    slag::Buffer* readback = device
                                 ->createBuffer({.size = elementCount * 4,
                                                 .placement = slag::Placement::automatic(slag::MemoryKind::Readback),
                                                 .debugName = "readback"})
                                 .value();
    REQUIRE(output);
    REQUIRE(readback);
    device->writeRWStructuredBuffer(outputSlot, {.buffer = output, .offset = 0, .count = elementCount, .stride = 4});

    slag::CommandBuffer* commandBuffer =
        device->createCommandBuffer({.queueType = slag::CommandQueueType::Compute, .debugName = "spike"}).value();
    REQUIRE(commandBuffer);
    REQUIRE(static_cast<bool>(commandBuffer->begin()));
    commandBuffer->bindPipeline(pipeline.value());
    struct
    {
        uint32_t bufferIndex;
        uint32_t count;
    } constants = {outputSlot, elementCount};
    commandBuffer->pushConstants(&constants, sizeof(constants));
    slag::Buffer* arguments = nullptr;
    if (indirect)
    {
        // The same group count, from a buffer (ADR 0047).
        arguments =
            device->createBuffer({.size = 256, .placement = slag::Placement::automatic(slag::MemoryKind::Upload)})
                .value();
        REQUIRE(arguments);
        *static_cast<slag::DispatchArguments*>(arguments->mappedData()) = {(elementCount + 63) / 64, 1, 1};
        slag::BufferBarrier toIndirect = {.syncBefore = slag::BarrierSync::None,
                                          .syncAfter = slag::BarrierSync::ExecuteIndirect,
                                          .accessAfter = slag::BarrierAccess::IndirectArgument,
                                          .buffer = arguments};
        commandBuffer->barrier({}, {&toIndirect, 1});
        commandBuffer->dispatchIndirect(arguments, 0);
    }
    else
    {
        commandBuffer->dispatch((elementCount + 63) / 64, 1, 1);
    }
    slag::BufferBarrier toCopy = {.syncBefore = slag::BarrierSync::ComputeShading,
                                  .syncAfter = slag::BarrierSync::Copy,
                                  .accessBefore = slag::BarrierAccess::ShaderWrite,
                                  .accessAfter = slag::BarrierAccess::CopySource,
                                  .buffer = output};
    commandBuffer->barrier({}, {&toCopy, 1});
    commandBuffer->copyBuffer(readback, 0, output, 0, elementCount * 4);
    REQUIRE(static_cast<bool>(commandBuffer->end()));

    slag::Semaphore* semaphore = device->createSemaphore({.debugName = "spike done"}).value();
    REQUIRE(semaphore);
    slag::SemaphoreValue signal = {semaphore, 1};
    slag::CommandBuffer* commandBuffers[] = {commandBuffer};
    REQUIRE(static_cast<bool>(device->commandQueue(slag::CommandQueueType::Compute)
                                  ->submit({.commandBuffers = commandBuffers, .signals = {&signal, 1}})));
    REQUIRE(static_cast<bool>(signal.semaphore->wait(signal.value, 5'000'000'000)));

    const auto* values = static_cast<const uint32_t*>(readback->mappedData());
    uint32_t mismatches = 0;
    for (uint32_t i = 0; i < elementCount; ++i)
    {
        mismatches += values[i] == i * 2 + 1 ? 0 : 1;
    }
    CHECK(mismatches == 0);
    CHECK(values[0] == 1);
    CHECK(values[elementCount - 1] == (elementCount - 1) * 2 + 1);

    device->destroy(arguments);
    device->destroy(semaphore);
    device->destroy(commandBuffer);
    device->destroy(output);
    device->destroy(readback);
    device->destroy(pipeline.value());
    device->shutdown();
    test.checkNoErrors();
}

// Pipeline cache (ADR 0043): a blob saved after creating a pipeline seeds
// the next initialize; a blob from elsewhere is discarded without errors.
void testPipelineCache(slag::Backend backend)
{
    TestInstance test(backend);
    slag::GraphicsDevice* device = test.firstDevice();
    REQUIRE(device);
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
    auto createAndDestroy = [&] {
        slag::Result<slag::Pipeline*> pipeline = device->createComputePipeline({.compute = {.binaries = binaries}});
        REQUIRE(static_cast<bool>(pipeline));
        device->destroy(pipeline.value());
    };

    REQUIRE(static_cast<bool>(device->initialize({})));
    createAndDestroy();
    size_t size = device->pipelineCacheData({});
    std::vector<std::byte> blob(size);
    CHECK(device->pipelineCacheData(blob) == size);
    MESSAGE("pipeline cache: " << size << " bytes");
    device->shutdown();

    REQUIRE(static_cast<bool>(device->initialize({.pipelineCacheData = blob})));
    createAndDestroy();
    CHECK(device->pipelineCacheData({}) >= size);
    device->shutdown();

    std::vector<std::byte> garbage(64, std::byte{0xAB});
    REQUIRE(static_cast<bool>(device->initialize({.pipelineCacheData = garbage})));
    createAndDestroy();
    device->shutdown();
    test.checkNoErrors();
}

// Submit boundaries (ADR 0049): a submit starts after every earlier submit on
// the queue has finished, with its writes visible, as at D3D12's
// ExecuteCommandLists boundaries. The second copy reads what the first wrote,
// with no barrier and no semaphore in between; synchronization validation
// reports the hazard on Vulkan otherwise. (Copies, because validation can't
// see shader accesses through the descriptor heap.)
void testSubmitBoundary(slag::Backend backend)
{
    TestInstance test(backend, slag::ValidationFailureAction::Continue,
                      slag::ValidationFlags::Api | slag::ValidationFlags::Synchronization);
    slag::GraphicsDevice* device = test.firstDevice();
    REQUIRE(device);
    REQUIRE(static_cast<bool>(device->initialize({})));
    constexpr uint32_t count = 4096;
    auto buffer = [&](slag::MemoryKind kind) {
        return device->createBuffer({.size = count * 4, .placement = slag::Placement::automatic(kind)}).value();
    };
    slag::Buffer* upload = buffer(slag::MemoryKind::Upload);
    slag::Buffer* middle = buffer(slag::MemoryKind::GpuOnly);
    slag::Buffer* readback = buffer(slag::MemoryKind::Readback);
    slag::CommandBuffer* first = device->createCommandBuffer({.queueType = slag::CommandQueueType::Copy}).value();
    slag::CommandBuffer* second = device->createCommandBuffer({.queueType = slag::CommandQueueType::Copy}).value();
    slag::Semaphore* done = device->createSemaphore({}).value();
    REQUIRE((upload && middle && readback && first && second && done));
    auto* words = static_cast<uint32_t*>(upload->mappedData());
    for (uint32_t i = 0; i < count; ++i)
    {
        words[i] = i * 2 + 1;
    }

    REQUIRE(static_cast<bool>(first->begin()));
    first->copyBuffer(middle, 0, upload, 0, count * 4);
    REQUIRE(static_cast<bool>(first->end()));
    REQUIRE(static_cast<bool>(second->begin()));
    second->copyBuffer(readback, 0, middle, 0, count * 4);
    REQUIRE(static_cast<bool>(second->end()));

    slag::CommandQueue* queue = device->commandQueue(slag::CommandQueueType::Copy);
    slag::CommandBuffer* firstList[] = {first};
    slag::CommandBuffer* secondList[] = {second};
    slag::SemaphoreValue signal = {done, 1};
    REQUIRE(static_cast<bool>(queue->submit({.commandBuffers = firstList})));
    REQUIRE(static_cast<bool>(queue->submit({.commandBuffers = secondList, .signals = {&signal, 1}})));
    REQUIRE(static_cast<bool>(done->wait(1, 5'000'000'000)));

    const auto* values = static_cast<const uint32_t*>(readback->mappedData());
    uint32_t mismatches = 0;
    for (uint32_t i = 0; i < count; ++i)
    {
        mismatches += values[i] == i * 2 + 1 ? 0 : 1;
    }
    CHECK(mismatches == 0);

    device->destroy(done);
    device->destroy(second);
    device->destroy(first);
    device->destroy(readback);
    device->destroy(middle);
    device->destroy(upload);
    device->shutdown();
    test.checkNoErrors();
}

// Every listed device: descriptor sizes differ between GPUs.
void runFillBufferOnAllDevices(slag::Backend backend)
{
    TestInstance test(backend);
    REQUIRE(test.instance);
    REQUIRE_FALSE(test.instance->graphicsDevices().empty());
    for (slag::GraphicsDevice* device : test.instance->graphicsDevices())
    {
        runFillBuffer(test, device, false);
        runFillBuffer(test, device, true);
    }
}

} // namespace

#ifdef SLAG_BACKEND_VULKAN
TEST_CASE("Vulkan: M1 spike - compute writes through the resource heap, CPU reads it back")
{
    runFillBufferOnAllDevices(slag::Backend::Vulkan);
}
#endif

#ifdef SLAG_BACKEND_D3D12
TEST_CASE("D3D12: M1 spike - compute writes through the resource heap, CPU reads it back")
{
    runFillBufferOnAllDevices(slag::Backend::D3D12);
}
#endif

#ifdef SLAG_BACKEND_VULKAN
TEST_CASE("Vulkan: a submit sees the finished work of earlier submits on its queue")
{
    testSubmitBoundary(slag::Backend::Vulkan);
}
#endif

#ifdef SLAG_BACKEND_D3D12
TEST_CASE("D3D12: a submit sees the finished work of earlier submits on its queue")
{
    testSubmitBoundary(slag::Backend::D3D12);
}
#endif

#ifdef SLAG_BACKEND_VULKAN
TEST_CASE("Vulkan: the pipeline cache blob round-trips and rejects foreign blobs")
{
    testPipelineCache(slag::Backend::Vulkan);
}
#endif

#ifdef SLAG_BACKEND_D3D12
TEST_CASE("D3D12: the pipeline cache blob round-trips and rejects foreign blobs")
{
    testPipelineCache(slag::Backend::D3D12);
}
#endif
