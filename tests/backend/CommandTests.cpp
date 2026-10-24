#include "TestDevice.h"

#include <slag/Buffer.h>
#include <slag/CommandBuffer.h>
#include <slag/GraphicsDevice.h>

#include <cstdint>
#include <cstring>

#ifdef SLAG_BACKEND_VULKAN
TEST_CASE("Vulkan: copy a buffer on each queue type and read it back")
{
    TestInstance test(slag::Backend::Vulkan);
    slag::GraphicsDevice* device = test.firstDevice();
    REQUIRE(device);
    REQUIRE(static_cast<bool>(device->initialize({})));

    constexpr uint32_t kCount = 1024;
    slag::Buffer* upload = device
                               ->createBuffer({.size = kCount * 4,
                                               .placement = slag::Placement::automatic(slag::MemoryKind::Upload),
                                               .debugName = "upload"})
                               .value();
    slag::Buffer* readback = device
                                 ->createBuffer({.size = kCount * 4,
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
        for (uint32_t i = 0; i < kCount; ++i)
        {
            source[i] = i * 3 + static_cast<uint32_t>(type);
        }
        std::memset(readback->mappedData(), 0, kCount * 4);

        slag::CommandBuffer* commandBuffer =
            device->createCommandBuffer({.queueType = type, .debugName = "copy"}).value();
        REQUIRE(commandBuffer);

        // Record twice to check reset and reuse (ADR 0045).
        for (int pass = 0; pass < 2; ++pass)
        {
            REQUIRE(static_cast<bool>(commandBuffer->begin()));
            commandBuffer->copyBuffer(readback, 0, upload, 0, kCount * 4);
            REQUIRE(static_cast<bool>(commandBuffer->end()));

            slag::SemaphoreValue signal = {semaphore, ++signalValue};
            slag::CommandBuffer* commandBuffers[] = {commandBuffer};
            slag::CommandQueue* queue = device->commandQueue(type);
            REQUIRE(static_cast<bool>(queue->submit({.commandBuffers = commandBuffers, .signals = {&signal, 1}})));
            REQUIRE(static_cast<bool>(device->waitSemaphores({&signal, 1}, slag::WaitMode::All, 5'000'000'000)));

            const auto* result = static_cast<const uint32_t*>(readback->mappedData());
            CHECK(std::memcmp(result, source, kCount * 4) == 0);
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
#endif
