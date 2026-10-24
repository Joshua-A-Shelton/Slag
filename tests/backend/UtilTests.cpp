#include "TestDevice.h"

#include <slag/AttachmentView.h>
#include <slag/Buffer.h>
#include <slag/CommandBuffer.h>
#include <slag/GraphicsDevice.h>
#include <slag/MemoryBlock.h>
#include <slag/QueryPool.h>
#include <slag/Semaphore.h>
#include <slag/Texture.h>
#include <slag/util/DeferredDestroyer.h>
#include <slag/util/DescriptorAllocator.h>

#include <cstdint>
#include <cstdlib>

namespace {

// DescriptorAllocator (ADR 0032): slot 0 is never handed out, an exhausted
// range returns 0, and freed slots come back only once their value is reached.
void testDescriptorAllocator(slag::Backend backend)
{
    TestInstance test(backend);
    slag::GraphicsDevice* device = test.firstDevice();
    REQUIRE(device);
    REQUIRE(static_cast<bool>(device->initialize({})));
    slag::Semaphore* semaphore = device->createSemaphore({}).value();
    REQUIRE(semaphore);
    {
        slag::DescriptorAllocator slots(device,
                                        {.heap = slag::DescriptorHeap::Resource, .firstSlot = 0, .slotCount = 4});
        CHECK(slots.allocate() == 1);
        CHECK(slots.allocate() == 2);
        CHECK(slots.allocate() == 3);
        CHECK(slots.allocate() == 0);
        slots.release(2, {semaphore, 1});
        slots.collect();
        CHECK(slots.allocate() == 0);
        REQUIRE(static_cast<bool>(semaphore->signal(1)));
        slots.collect();
        CHECK(slots.allocate() == 2);
        CHECK(slots.allocate() == 0);
    }
    {
        slag::DescriptorAllocator samplers(device,
                                           {.heap = slag::DescriptorHeap::Sampler, .firstSlot = 10, .slotCount = 2});
        CHECK(samplers.allocate() == 10);
        CHECK(samplers.allocate() == 11);
        CHECK(samplers.allocate() == 0);
    }
    device->destroy(semaphore);
    device->shutdown();
    test.checkNoErrors();
}

// Counts the instance's live host allocations, to see objects being destroyed.
struct AllocationCounter
{
    int64_t live = 0;
};

void* countedAllocate(size_t size, size_t alignment, void* userData)
{
    ++static_cast<AllocationCounter*>(userData)->live;
#ifdef _WIN32
    return _aligned_malloc(size, alignment);
#else
    return std::aligned_alloc(alignment, (size + alignment - 1) / alignment * alignment);
#endif
}

void countedFree(void* pointer, size_t, void* userData)
{
    if (pointer)
    {
        --static_cast<AllocationCounter*>(userData)->live;
    }
#ifdef _WIN32
    _aligned_free(pointer);
#else
    std::free(pointer);
#endif
}

// DeferredDestroyer (ADR 0069): nothing is destroyed before its value is
// reached; collect() destroys what is reached, in an order that keeps views
// before textures and placed buffers before their block; flush() waits.
void testDeferredDestroyer(slag::Backend backend)
{
    AllocationCounter counter;
    TestMessages messages;
    slag::InstanceDesc desc;
    desc.backend = backend;
    desc.validationFlags = slag::ValidationFlags::Api;
    desc.validationFailureAction = slag::ValidationFailureAction::Continue;
    desc.allowSoftwareDevices = true;
    desc.messageCallback = collectTestMessage;
    desc.messageUserData = &messages;
    desc.allocatorCallbacks = {countedAllocate, countedFree, &counter};
    slag::Instance* instance = slag::createInstance(desc).value();
    REQUIRE(instance);
    REQUIRE_FALSE(instance->graphicsDevices().empty());
    slag::GraphicsDevice* device = instance->graphicsDevices()[0];
    REQUIRE(static_cast<bool>(device->initialize({})));

    slag::Semaphore* frame = device->createSemaphore({.debugName = "frame"}).value();
    REQUIRE(frame);
    {
        slag::DeferredDestroyer destroyer(device);
        int64_t baseline = counter.live;

        // Reached at 1: a texture and its view, a block and a buffer placed in it.
        slag::Texture* texture = device
                                     ->createTexture2D({.format = slag::PixelFormat::R8G8B8A8Unorm,
                                                        .width = 64,
                                                        .height = 64,
                                                        .usage = slag::TextureUsage::RenderTarget})
                                     .value();
        slag::AttachmentView* view = device->createAttachmentView({.texture = texture}).value();
        slag::MemoryBlock* block = device->createMemoryBlock({.size = 1 << 20}).value();
        slag::Buffer* placed =
            device->createBuffer({.size = 4096, .placement = slag::Placement::placed(block, 0)}).value();
        // Reached at 2: everything else.
        slag::Buffer* buffer =
            device->createBuffer({.size = 4096, .placement = slag::Placement::automatic(slag::MemoryKind::Upload)})
                .value();
        slag::QueryPool* pool = device->createQueryPool({.count = 4}).value();
        slag::CommandBuffer* commandBuffer = device->createCommandBuffer({}).value();
        slag::Semaphore* extra = device->createSemaphore({}).value();
        REQUIRE((texture && view && block && placed && buffer && pool && commandBuffer && extra));
        int64_t created = counter.live;
        CHECK(created > baseline);

        // In an order collect() must fix: the block before its buffer, the
        // texture before its view.
        destroyer.destroyAfter(block, {frame, 1});
        destroyer.destroyAfter(texture, {frame, 1});
        destroyer.destroyAfter(placed, {frame, 1});
        destroyer.destroyAfter(view, {frame, 1});
        destroyer.destroyAfter(buffer, {frame, 2});
        destroyer.destroyAfter(pool, {frame, 2});
        destroyer.destroyAfter(commandBuffer, {frame, 2});
        destroyer.destroyAfter(extra, {frame, 2});
        destroyer.destroyAfter(static_cast<slag::Buffer*>(nullptr), {frame, 1});

        destroyer.collect();
        CHECK(counter.live >= created); // nothing reached yet

        REQUIRE(static_cast<bool>(frame->signal(1)));
        destroyer.collect();
        int64_t afterFirst = counter.live;
        CHECK(afterFirst < created);

        // Reached before flush(), so its wait returns at once.
        REQUIRE(static_cast<bool>(frame->signal(2)));
        destroyer.flush();
        CHECK(counter.live < afterFirst);
    }
    device->destroy(frame);
    device->shutdown();
    slag::destroy(instance);
    CHECK(counter.live == 0);
    for (const std::string& error : messages.errors)
    {
        MESSAGE("error: " << error);
    }
    CHECK(messages.errors.empty());
}

} // namespace

#ifdef SLAG_BACKEND_VULKAN
TEST_CASE("Vulkan: DescriptorAllocator hands out and recycles slots")
{
    testDescriptorAllocator(slag::Backend::Vulkan);
}
TEST_CASE("Vulkan: DeferredDestroyer destroys objects once their values are reached")
{
    testDeferredDestroyer(slag::Backend::Vulkan);
}
#endif

#ifdef SLAG_BACKEND_D3D12
TEST_CASE("D3D12: DescriptorAllocator hands out and recycles slots")
{
    testDescriptorAllocator(slag::Backend::D3D12);
}
TEST_CASE("D3D12: DeferredDestroyer destroys objects once their values are reached")
{
    testDeferredDestroyer(slag::Backend::D3D12);
}
#endif
