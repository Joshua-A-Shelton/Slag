#include "TestDevice.h"

#include <slag/CommandQueue.h>
#include <slag/GraphicsDevice.h>
#include <slag/Semaphore.h>

#ifdef SLAG_BACKEND_VULKAN
TEST_CASE("Vulkan: initialize gives the requested queues and shutdown undoes it")
{
    TestInstance test(slag::Backend::Vulkan);
    slag::GraphicsDevice* device = test.firstDevice();
    REQUIRE(device);

    slag::GraphicsDeviceInitDesc desc;
    desc.computeQueueCount = 2;
    REQUIRE(static_cast<bool>(device->initialize(desc)));
    CHECK(device->status() == slag::Status::Ok);
    REQUIRE(device->commandQueue(slag::CommandQueueType::Graphics));
    CHECK(device->commandQueue(slag::CommandQueueType::Graphics)->type() == slag::CommandQueueType::Graphics);
    REQUIRE(device->commandQueue(slag::CommandQueueType::Compute, 1));
    REQUIRE(device->commandQueue(slag::CommandQueueType::Copy));
    CHECK(static_cast<bool>(device->commandQueue(slag::CommandQueueType::Copy)->waitIdle()));
    CHECK(static_cast<bool>(device->waitIdle()));
    device->shutdown();

    // A device can be initialized again, e.g. after device loss (ADR 0054).
    desc.resourceHeapSize = slag::limits::maxResourceHeapSize;
    REQUIRE(static_cast<bool>(device->initialize(desc)));
    device->shutdown();
    test.checkNoErrors();
}

TEST_CASE("Vulkan: more queues than the hardware has share hardware queues")
{
    TestInstance test(slag::Backend::Vulkan);
    slag::GraphicsDevice* device = test.firstDevice();
    REQUIRE(device);

    slag::GraphicsDeviceInitDesc desc;
    desc.graphicsQueueCount = 4;
    desc.computeQueueCount = 32;
    desc.copyQueueCount = 32;
    REQUIRE(static_cast<bool>(device->initialize(desc)));
    for (uint32_t i = 0; i < 32; ++i)
    {
        REQUIRE(device->commandQueue(slag::CommandQueueType::Compute, i));
        CHECK(static_cast<bool>(device->commandQueue(slag::CommandQueueType::Compute, i)->waitIdle()));
    }
    device->shutdown();
    test.checkNoErrors();
}

TEST_CASE("Vulkan: semaphores signal from the CPU and wait with timeouts")
{
    TestInstance test(slag::Backend::Vulkan);
    slag::GraphicsDevice* device = test.firstDevice();
    REQUIRE(device);
    REQUIRE(static_cast<bool>(device->initialize({})));

    slag::Result<slag::Semaphore*> a = device->createSemaphore({.initialValue = 5, .debugName = "a"});
    slag::Result<slag::Semaphore*> b = device->createSemaphore({.debugName = "b"});
    REQUIRE(static_cast<bool>(a));
    REQUIRE(static_cast<bool>(b));
    CHECK(a.value()->value() == 5);
    CHECK(b.value()->value() == 0);

    slag::SemaphoreValue waits[] = {{a.value(), 5}, {b.value(), 1}};
    CHECK(device->waitSemaphores(waits, slag::WaitMode::All, 1'000'000).status() == slag::Status::Timeout);
    CHECK(device->waitSemaphores(waits, slag::WaitMode::Any, 0).status() == slag::Status::Ok);

    REQUIRE(static_cast<bool>(b.value()->signal(1)));
    CHECK(b.value()->value() == 1);
    CHECK(device->waitSemaphores(waits, slag::WaitMode::All, 0).status() == slag::Status::Ok);

    device->destroy(a.value());
    device->destroy(b.value());
    device->shutdown();
    test.checkNoErrors();
}

#ifdef SLAG_VALIDATION
TEST_CASE("Vulkan: validation reports a semaphore value that doesn't increase")
{
    TestInstance test(slag::Backend::Vulkan);
    slag::GraphicsDevice* device = test.firstDevice();
    REQUIRE(device);
    REQUIRE(static_cast<bool>(device->initialize({})));
    slag::Result<slag::Semaphore*> semaphore = device->createSemaphore({.initialValue = 3});
    REQUIRE(static_cast<bool>(semaphore));

    (void)semaphore.value()->signal(3);
    REQUIRE_FALSE(test.messages.errors.empty());
    CHECK(test.messages.errors[0].find("Semaphore::signal") != std::string::npos);

    device->destroy(semaphore.value());
    device->shutdown();
}
#endif
#endif
