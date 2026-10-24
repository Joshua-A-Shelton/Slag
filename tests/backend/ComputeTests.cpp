#include "TestDevice.h"

#include <slag/Buffer.h>
#include <slag/CommandBuffer.h>
#include <slag/GraphicsDevice.h>
#include <slag/Pipeline.h>

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
void runFillBuffer(TestInstance& test, slag::GraphicsDevice* device)
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
    commandBuffer->dispatch((elementCount + 63) / 64, 1, 1);
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
    REQUIRE(static_cast<bool>(device->waitSemaphores({&signal, 1}, slag::WaitMode::All, 5'000'000'000)));

    const auto* values = static_cast<const uint32_t*>(readback->mappedData());
    uint32_t mismatches = 0;
    for (uint32_t i = 0; i < elementCount; ++i)
    {
        mismatches += values[i] == i * 2 + 1 ? 0 : 1;
    }
    CHECK(mismatches == 0);
    CHECK(values[0] == 1);
    CHECK(values[elementCount - 1] == (elementCount - 1) * 2 + 1);

    device->destroy(semaphore);
    device->destroy(commandBuffer);
    device->destroy(output);
    device->destroy(readback);
    device->destroy(pipeline.value());
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
        runFillBuffer(test, device);
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
