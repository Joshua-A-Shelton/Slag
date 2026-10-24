#include "TestDevice.h"

#include <slag/Buffer.h>
#include <slag/CommandBuffer.h>
#include <slag/GraphicsDevice.h>

#include <slag/Pipeline.h>

#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace {

void testCopyOnEachQueue(slag::Backend backend)
{
    TestInstance test(backend);
    slag::GraphicsDevice* device = test.firstDevice();
    REQUIRE(device);
    REQUIRE(static_cast<bool>(device->initialize({})));

    constexpr uint32_t elementCount = 1024;
    slag::Buffer* upload = device
                               ->createBuffer({.size = elementCount * 4,
                                               .placement = slag::Placement::automatic(slag::MemoryKind::Upload),
                                               .debugName = "upload"})
                               .value();
    slag::Buffer* readback = device
                                 ->createBuffer({.size = elementCount * 4,
                                                 .placement = slag::Placement::automatic(slag::MemoryKind::Readback),
                                                 .debugName = "readback"})
                                 .value();
    REQUIRE(upload);
    REQUIRE(readback);
    slag::Semaphore* semaphore = device->createSemaphore({.debugName = "copy done"}).value();
    REQUIRE(semaphore);

    uint64_t signalValue = 0;
    for (slag::CommandQueueType type :
         {slag::CommandQueueType::Graphics, slag::CommandQueueType::Compute, slag::CommandQueueType::Copy})
    {
        auto* source = static_cast<uint32_t*>(upload->mappedData());
        for (uint32_t i = 0; i < elementCount; ++i)
        {
            source[i] = i * 3 + static_cast<uint32_t>(type);
        }
        std::memset(readback->mappedData(), 0, elementCount * 4);

        slag::CommandBuffer* commandBuffer =
            device->createCommandBuffer({.queueType = type, .debugName = "copy"}).value();
        REQUIRE(commandBuffer);

        // Record twice to check reset and reuse (ADR 0045).
        for (int pass = 0; pass < 2; ++pass)
        {
            REQUIRE(static_cast<bool>(commandBuffer->begin()));
            commandBuffer->copyBuffer(readback, 0, upload, 0, elementCount * 4);
            REQUIRE(static_cast<bool>(commandBuffer->end()));

            slag::SemaphoreValue signal = {semaphore, ++signalValue};
            slag::CommandBuffer* commandBuffers[] = {commandBuffer};
            slag::CommandQueue* queue = device->commandQueue(type);
            REQUIRE(static_cast<bool>(queue->submit({.commandBuffers = commandBuffers, .signals = {&signal, 1}})));
            REQUIRE(static_cast<bool>(device->waitSemaphores({&signal, 1}, slag::WaitMode::All, 5'000'000'000)));

            const auto* result = static_cast<const uint32_t*>(readback->mappedData());
            CHECK(std::memcmp(result, source, elementCount * 4) == 0);
            REQUIRE(static_cast<bool>(commandBuffer->reset()));
        }
        device->destroy(commandBuffer);
    }

    device->destroy(semaphore);
    device->destroy(upload);
    device->destroy(readback);
    device->shutdown();
    test.checkNoErrors();
}

std::vector<char> readFile(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

// Compute fills a buffer; the copy queue waits on the compute queue's
// semaphore and copies it out; the graphics queue waits on the copy. The
// compute submit also waits on a gate the CPU signals only after every queue
// has its work, so the queues wait before the value is signaled (ADR 0054).
void testQueueChain(slag::Backend backend)
{
    TestInstance test(backend);
    slag::GraphicsDevice* device = test.firstDevice();
    REQUIRE(device);
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
    slag::Pipeline* pipeline = device->createComputePipeline({.compute = {.binaries = binaries}}).value();
    REQUIRE(pipeline);

    constexpr uint32_t elementCount = 4096;
    constexpr uint32_t slot = 7;
    slag::Buffer* filled = device
                               ->createBuffer({.size = elementCount * 4,
                                               .placement = slag::Placement::automatic(slag::MemoryKind::GpuOnly),
                                               .debugName = "filled"})
                               .value();
    slag::Buffer* copied = device
                               ->createBuffer({.size = elementCount * 4,
                                               .placement = slag::Placement::automatic(slag::MemoryKind::GpuOnly),
                                               .debugName = "copied"})
                               .value();
    slag::Buffer* readback = device
                                 ->createBuffer({.size = elementCount * 4,
                                                 .placement = slag::Placement::automatic(slag::MemoryKind::Readback),
                                                 .debugName = "readback"})
                                 .value();
    REQUIRE((filled && copied && readback));
    device->writeRWStructuredBuffer(slot, {.buffer = filled, .offset = 0, .count = elementCount, .stride = 4});

    slag::Semaphore* gate = device->createSemaphore({.debugName = "gate"}).value();
    slag::Semaphore* computeDone = device->createSemaphore({.debugName = "compute done"}).value();
    slag::Semaphore* copyDone = device->createSemaphore({.debugName = "copy done"}).value();
    slag::Semaphore* graphicsDone = device->createSemaphore({.debugName = "graphics done"}).value();
    REQUIRE((gate && computeDone && copyDone && graphicsDone));

    slag::CommandBuffer* compute = device->createCommandBuffer({.queueType = slag::CommandQueueType::Compute}).value();
    slag::CommandBuffer* copy = device->createCommandBuffer({.queueType = slag::CommandQueueType::Copy}).value();
    slag::CommandBuffer* graphics =
        device->createCommandBuffer({.queueType = slag::CommandQueueType::Graphics}).value();
    REQUIRE((compute && copy && graphics));

    REQUIRE(static_cast<bool>(compute->begin()));
    compute->bindPipeline(pipeline);
    uint32_t constants[] = {slot, elementCount};
    compute->pushConstants(constants, sizeof(constants));
    compute->dispatch((elementCount + 63) / 64, 1, 1);
    REQUIRE(static_cast<bool>(compute->end()));

    REQUIRE(static_cast<bool>(copy->begin()));
    copy->copyBuffer(copied, 0, filled, 0, elementCount * 4);
    REQUIRE(static_cast<bool>(copy->end()));

    REQUIRE(static_cast<bool>(graphics->begin()));
    graphics->copyBuffer(readback, 0, copied, 0, elementCount * 4);
    REQUIRE(static_cast<bool>(graphics->end()));

    slag::SemaphoreValue gateWait = {gate, 1};
    slag::SemaphoreValue computeSignal = {computeDone, 1};
    slag::SemaphoreValue copySignal = {copyDone, 1};
    slag::SemaphoreValue graphicsSignal = {graphicsDone, 1};
    slag::CommandBuffer* computeList[] = {compute};
    slag::CommandBuffer* copyList[] = {copy};
    slag::CommandBuffer* graphicsList[] = {graphics};
    // Submitted in reverse, so each queue waits on work not yet submitted.
    REQUIRE(static_cast<bool>(
        device->commandQueue(slag::CommandQueueType::Graphics)
            ->submit({.waits = {&copySignal, 1}, .commandBuffers = graphicsList, .signals = {&graphicsSignal, 1}})));
    REQUIRE(static_cast<bool>(
        device->commandQueue(slag::CommandQueueType::Copy)
            ->submit({.waits = {&computeSignal, 1}, .commandBuffers = copyList, .signals = {&copySignal, 1}})));
    REQUIRE(static_cast<bool>(
        device->commandQueue(slag::CommandQueueType::Compute)
            ->submit({.waits = {&gateWait, 1}, .commandBuffers = computeList, .signals = {&computeSignal, 1}})));

    CHECK(device->waitSemaphores({&graphicsSignal, 1}, slag::WaitMode::All, 50'000'000).status() ==
          slag::Status::Timeout);
    CHECK(graphicsDone->value() == 0);
    REQUIRE(static_cast<bool>(gate->signal(1)));
    REQUIRE(static_cast<bool>(device->waitSemaphores({&graphicsSignal, 1}, slag::WaitMode::All, 5'000'000'000)));

    const auto* values = static_cast<const uint32_t*>(readback->mappedData());
    uint32_t mismatches = 0;
    for (uint32_t i = 0; i < elementCount; ++i)
    {
        mismatches += values[i] == i * 2 + 1 ? 0 : 1;
    }
    CHECK(mismatches == 0);

    device->destroy(graphics);
    device->destroy(copy);
    device->destroy(compute);
    device->destroy(graphicsDone);
    device->destroy(copyDone);
    device->destroy(computeDone);
    device->destroy(gate);
    device->destroy(readback);
    device->destroy(copied);
    device->destroy(filled);
    device->destroy(pipeline);
    device->shutdown();
    test.checkNoErrors();
}

} // namespace

#ifdef SLAG_BACKEND_VULKAN
TEST_CASE("Vulkan: copy a buffer on each queue type and read it back")
{
    testCopyOnEachQueue(slag::Backend::Vulkan);
}
TEST_CASE("Vulkan: compute, copy and graphics queues chained by semaphores")
{
    testQueueChain(slag::Backend::Vulkan);
}
#endif

#ifdef SLAG_BACKEND_D3D12
TEST_CASE("D3D12: copy a buffer on each queue type and read it back")
{
    testCopyOnEachQueue(slag::Backend::D3D12);
}
TEST_CASE("D3D12: compute, copy and graphics queues chained by semaphores")
{
    testQueueChain(slag::Backend::D3D12);
}
#endif
