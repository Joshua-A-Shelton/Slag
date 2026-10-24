#include "TestDevice.h"

#include <slag/Buffer.h>
#include <slag/CommandBuffer.h>
#include <slag/GraphicsDevice.h>
#include <slag/MemoryBlock.h>
#include <slag/Semaphore.h>

#include <cstdint>
#include <cstring>

namespace {

uint64_t alignUp(uint64_t value, uint64_t alignment)
{
    return (value + alignment - 1) / alignment * alignment;
}

// Upload block -> GpuOnly block -> Readback block, all placed buffers.
void testPlacedBuffers(slag::Backend backend)
{
    TestInstance test(backend);
    slag::GraphicsDevice* device = test.firstDevice();
    REQUIRE(device);
    REQUIRE(static_cast<bool>(device->initialize({})));

    constexpr uint64_t size = 300000; // deliberately not a multiple of 64 KB
    slag::PlacementInfo placement = device->placementInfo(slag::BufferDesc{.size = size});
    CHECK(placement.size >= size);
    CHECK(placement.alignment >= 65536);
    CHECK(placement.alignment % 65536 == 0);
    uint64_t second = alignUp(placement.size, placement.alignment);

    slag::MemoryBlock* upload =
        device->createMemoryBlock({.size = 4 << 20, .kind = slag::MemoryKind::Upload, .debugName = "upload block"})
            .value();
    slag::MemoryBlock* gpu =
        device->createMemoryBlock({.size = 4 << 20, .kind = slag::MemoryKind::GpuOnly, .userData = 3}).value();
    slag::MemoryBlock* readback =
        device->createMemoryBlock({.size = 4 << 20, .kind = slag::MemoryKind::Readback}).value();
    REQUIRE(upload);
    REQUIRE(gpu);
    REQUIRE(readback);
    CHECK(gpu->size() == 4 << 20);
    CHECK(gpu->kind() == slag::MemoryKind::GpuOnly);
    CHECK(gpu->userData() == 3);

    // Two buffers per block, the second one after the first.
    auto place = [&](slag::MemoryBlock* block, uint64_t offset) {
        return device->createBuffer({.size = size, .placement = slag::Placement::placed(block, offset)}).value();
    };
    slag::Buffer* source = place(upload, second);
    slag::Buffer* scratch = place(gpu, 0);
    slag::Buffer* middle = place(gpu, second);
    slag::Buffer* destination = place(readback, 0);
    REQUIRE(source);
    REQUIRE(scratch);
    REQUIRE(middle);
    REQUIRE(destination);
    CHECK(middle->gpuAddress() != scratch->gpuAddress());
    CHECK(middle->mappedData() == nullptr);
    REQUIRE(source->mappedData());
    REQUIRE(destination->mappedData());

    auto* bytes = static_cast<uint8_t*>(source->mappedData());
    for (uint64_t i = 0; i < size; ++i)
    {
        bytes[i] = static_cast<uint8_t>(i * 7 + 1);
    }

    slag::CommandBuffer* commandBuffer =
        device->createCommandBuffer({.queueType = slag::CommandQueueType::Graphics}).value();
    REQUIRE(commandBuffer);
    REQUIRE(static_cast<bool>(commandBuffer->begin()));
    commandBuffer->copyBuffer(middle, 0, source, 0, size);
    slag::BufferBarrier barrier = {.syncBefore = slag::BarrierSync::Copy,
                                   .syncAfter = slag::BarrierSync::Copy,
                                   .accessBefore = slag::BarrierAccess::CopyDest,
                                   .accessAfter = slag::BarrierAccess::CopySource,
                                   .buffer = middle};
    commandBuffer->barrier({}, {&barrier, 1});
    commandBuffer->copyBuffer(destination, 0, middle, 0, size);
    REQUIRE(static_cast<bool>(commandBuffer->end()));

    slag::Semaphore* semaphore = device->createSemaphore({}).value();
    slag::SemaphoreValue done = {semaphore, 1};
    slag::CommandBuffer* commandBuffers[] = {commandBuffer};
    REQUIRE(static_cast<bool>(device->commandQueue(slag::CommandQueueType::Graphics)
                                  ->submit({.commandBuffers = commandBuffers, .signals = {&done, 1}})));
    REQUIRE(static_cast<bool>(device->waitSemaphores({&done, 1}, slag::WaitMode::All, 5'000'000'000)));
    CHECK(std::memcmp(destination->mappedData(), source->mappedData(), size) == 0);

    device->destroy(semaphore);
    device->destroy(commandBuffer);
    for (slag::Buffer* buffer : {source, scratch, middle, destination})
    {
        device->destroy(buffer);
    }
    for (slag::MemoryBlock* block : {upload, gpu, readback})
    {
        device->destroy(block);
    }
    device->shutdown();
    test.checkNoErrors();
}

void testBlockValidation(slag::Backend backend)
{
    TestInstance test(backend);
    slag::GraphicsDevice* device = test.firstDevice();
    REQUIRE(device);
    REQUIRE(static_cast<bool>(device->initialize({})));

    slag::MemoryBlock* block = device->createMemoryBlock({.size = 1 << 20}).value();
    REQUIRE(block);
    slag::Buffer* buffer = device->createBuffer({.size = 4096, .placement = slag::Placement::placed(block, 0)}).value();
    REQUIRE(buffer);
    // Aliasing: another buffer over the same memory is allowed (ADR 0023).
    slag::Buffer* alias = device->createBuffer({.size = 4096, .placement = slag::Placement::placed(block, 0)}).value();
    REQUIRE(alias);
    device->destroy(alias);

    // Misaligned and out-of-range placements are validation errors.
    test.messages.errors.clear();
    slag::Result<slag::Buffer*> misaligned =
        device->createBuffer({.size = 4096, .placement = slag::Placement::placed(block, 4)});
    CHECK_FALSE(test.messages.errors.empty());
    if (misaligned)
    {
        device->destroy(misaligned.value());
    }
#ifdef SLAG_VALIDATION
    test.messages.errors.clear();
    device->destroy(buffer);
    device->destroy(block); // nothing left in it: no error
    CHECK(test.messages.errors.empty());
#else
    device->destroy(buffer);
    device->destroy(block);
#endif
    device->shutdown();
}

} // namespace

#ifdef SLAG_BACKEND_VULKAN
TEST_CASE("Vulkan: placed buffers in Upload, GpuOnly, and Readback memory blocks")
{
    testPlacedBuffers(slag::Backend::Vulkan);
}
#endif

#ifdef SLAG_BACKEND_D3D12
TEST_CASE("D3D12: placed buffers in Upload, GpuOnly, and Readback memory blocks")
{
    testPlacedBuffers(slag::Backend::D3D12);
}
#endif

#ifdef SLAG_VALIDATION
#ifdef SLAG_BACKEND_VULKAN
TEST_CASE("Vulkan: memory block validation")
{
    testBlockValidation(slag::Backend::Vulkan);
}
#endif

#ifdef SLAG_BACKEND_D3D12
TEST_CASE("D3D12: memory block validation")
{
    testBlockValidation(slag::Backend::D3D12);
}
#endif
#endif
