#include "TestDevice.h"

#include <slag/Buffer.h>
#include <slag/CommandBuffer.h>
#include <slag/CommandQueue.h>
#include <slag/GraphicsDevice.h>
#include <slag/QueryPool.h>
#include <slag/Semaphore.h>

#include <cstdint>
#include <string>

namespace {

const char* queueName(slag::CommandQueueType type)
{
    switch (type)
    {
    case slag::CommandQueueType::Graphics:
        return "graphics";
    case slag::CommandQueueType::Compute:
        return "compute";
    case slag::CommandQueueType::Copy:
        return "copy";
    }
    return "?";
}

// Timestamps around a copy, twice through the same pool (the second round
// checks that a resolve leaves the queries writable again), bracketed by
// calibrations on the same GPU clock.
void timestampQueue(slag::GraphicsDevice* device, slag::CommandQueueType type)
{
    MESSAGE("queue: " << std::string(queueName(type)));
    const slag::GraphicsDeviceCapabilities& capabilities = device->capabilities();
    slag::CommandQueue* queue = device->commandQueue(type);
    REQUIRE(queue);
    uint64_t frequency = queue->timestampFrequency();
    MESSAGE("frequency: " << frequency << " Hz, valid bits: " << capabilities.timestampValidBits);
    CHECK(frequency > 0);

    constexpr uint64_t copySize = 16 << 20;
    slag::Buffer* source = device
                               ->createBuffer({.size = copySize,
                                               .placement = slag::Placement::automatic(slag::MemoryKind::GpuOnly),
                                               .debugName = "source"})
                               .value();
    slag::Buffer* destination = device
                                    ->createBuffer({.size = copySize,
                                                    .placement = slag::Placement::automatic(slag::MemoryKind::GpuOnly),
                                                    .debugName = "destination"})
                                    .value();
    slag::Buffer* results = device
                                ->createBuffer({.size = 4 * sizeof(uint64_t),
                                                .placement = slag::Placement::automatic(slag::MemoryKind::Readback),
                                                .debugName = "timestamps"})
                                .value();
    slag::QueryPool* pool = device->createQueryPool({.count = 2, .queueType = type, .debugName = "timestamps"}).value();
    slag::CommandBuffer* commandBuffer = device->createCommandBuffer({.queueType = type}).value();
    slag::Semaphore* semaphore = device->createSemaphore({}).value();
    REQUIRE(source);
    REQUIRE(destination);
    REQUIRE(results);
    REQUIRE(pool);
    REQUIRE(commandBuffer);
    REQUIRE(semaphore);

    slag::TimestampCalibration before = {};
    if (capabilities.timestampCalibration)
    {
        slag::Result<slag::TimestampCalibration> calibration = queue->calibrateTimestamps();
        REQUIRE(static_cast<bool>(calibration));
        before = calibration.value();
    }

    for (uint64_t round = 0; round < 2; ++round)
    {
        REQUIRE(static_cast<bool>(commandBuffer->begin()));
        commandBuffer->writeTimestamp(pool, 0);
        commandBuffer->copyBuffer(destination, 0, source, 0, copySize);
        commandBuffer->writeTimestamp(pool, 1);
        commandBuffer->resolveQueries(pool, 0, 2, results, round * 2 * sizeof(uint64_t));
        REQUIRE(static_cast<bool>(commandBuffer->end()));
        slag::CommandBuffer* commandBuffers[] = {commandBuffer};
        slag::SemaphoreValue signal = {semaphore, round + 1};
        REQUIRE(static_cast<bool>(queue->submit({.commandBuffers = commandBuffers, .signals = {&signal, 1}})));
        REQUIRE(static_cast<bool>(signal.semaphore->wait(signal.value, 5'000'000'000)));
        REQUIRE(static_cast<bool>(commandBuffer->reset()));
    }

    const auto* values = static_cast<const uint64_t*>(results->mappedData());
    uint64_t mask =
        capabilities.timestampValidBits >= 64 ? UINT64_MAX : (uint64_t(1) << capabilities.timestampValidBits) - 1;
    for (int round = 0; round < 2; ++round)
    {
        uint64_t start = values[round * 2] & mask;
        uint64_t end = values[round * 2 + 1] & mask;
        double microseconds = double(end - start) * 1e6 / double(frequency);
        MESSAGE("round " << round << ": 16 MB copy took " << microseconds << " us");
        CHECK(end >= start);
        CHECK(microseconds < 1e6);
    }
    CHECK((values[2] & mask) >= (values[1] & mask));

    if (capabilities.timestampCalibration)
    {
        slag::Result<slag::TimestampCalibration> calibration = queue->calibrateTimestamps();
        REQUIRE(static_cast<bool>(calibration));
        slag::TimestampCalibration after = calibration.value();
        CHECK(after.cpuNanoseconds > before.cpuNanoseconds);
        CHECK(after.gpuTimestamp > before.gpuTimestamp);
        // The recorded work ran between the two calibrations.
        CHECK((values[0] & mask) >= (before.gpuTimestamp & mask));
        CHECK((values[3] & mask) <= (after.gpuTimestamp & mask));
        // Both clocks advance at the same rate, within 1% over this span.
        double gpuSeconds = double(after.gpuTimestamp - before.gpuTimestamp) / double(frequency);
        double cpuSeconds = double(after.cpuNanoseconds - before.cpuNanoseconds) * 1e-9;
        MESSAGE("calibration span: GPU " << gpuSeconds * 1e3 << " ms, CPU " << cpuSeconds * 1e3 << " ms");
        CHECK(gpuSeconds == doctest::Approx(cpuSeconds).epsilon(0.01).scale(0.001));
    }

    device->destroy(semaphore);
    device->destroy(commandBuffer);
    device->destroy(pool);
    device->destroy(results);
    device->destroy(destination);
    device->destroy(source);
}

void testTimestamps(slag::Backend backend)
{
    TestInstance test(backend);
    slag::GraphicsDevice* device = test.firstDevice();
    REQUIRE(device);
    MESSAGE("device: " << std::string(device->info().name));
    REQUIRE(static_cast<bool>(device->initialize({})));
    const slag::GraphicsDeviceCapabilities& capabilities = device->capabilities();
    MESSAGE("copy-queue timestamps: " << capabilities.copyQueueTimestamps
                                      << ", calibration: " << capabilities.timestampCalibration);
    timestampQueue(device, slag::CommandQueueType::Graphics);
    timestampQueue(device, slag::CommandQueueType::Compute);
    if (capabilities.copyQueueTimestamps)
    {
        timestampQueue(device, slag::CommandQueueType::Copy);
    }
    device->shutdown();
    test.checkNoErrors();
}

} // namespace

#ifdef SLAG_BACKEND_VULKAN
TEST_CASE("Vulkan: timestamps measure work on every queue type")
{
    testTimestamps(slag::Backend::Vulkan);
}
#endif

#ifdef SLAG_BACKEND_D3D12
TEST_CASE("D3D12: timestamps measure work on every queue type")
{
    testTimestamps(slag::Backend::D3D12);
}
#endif
