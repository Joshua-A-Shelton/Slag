// Validation of devices, queues, and semaphores.

#include "ValidationFixture.h"

using namespace slag;

TEST_CASE("device initialization and queries")
{
    ValidationFixture f(false);
    CHECK_VALIDATION(f, "exceeds limits::maxResourceHeapSize",
                     (void)f.device->initialize({.resourceHeapSize = limits::maxResourceHeapSize + 1}));
    f.device->shutdown();
    CHECK_VALIDATION(f, "exceeds limits::maxSamplerHeapSize",
                     (void)f.device->initialize({.samplerHeapSize = limits::maxSamplerHeapSize + 1}));
    f.device->shutdown();
    CHECK_VALIDATION(f, "graphicsQueueCount must be at least 1", (void)f.device->initialize({.graphicsQueueCount = 0}));
    f.device->shutdown();
    REQUIRE(static_cast<bool>(f.device->initialize({})));
    CHECK_VALIDATION(f, "is already initialized", (void)f.device->initialize({}));
    CHECK_VALIDATION(f, "is not a PixelFormat", (void)f.device->formatSupport(static_cast<PixelFormat>(0xFFFF)));
    CHECK_VALIDATION(f, "queue index 1 is out of range", (void)f.device->commandQueue(CommandQueueType::Graphics, 1));
}

TEST_CASE("semaphores")
{
    ValidationFixture f;
    Semaphore* semaphore = f.device->createSemaphore({.initialValue = 3}).value();
    CHECK_VALIDATION(f, "values must strictly increase", (void)semaphore->signal(3));
    SemaphoreValue none = {nullptr, 1};
    CHECK_VALIDATION(f, "a semaphore is null", (void)f.device->waitSemaphores({&none, 1}, WaitMode::All, 0));
    ValidationFixture other;
    Semaphore* foreign = other.device->createSemaphore({}).value();
    SemaphoreValue foreignValue = {foreign, 0};
    CHECK_VALIDATION(f, "a semaphore belongs to another device",
                     (void)f.device->waitSemaphores({&foreignValue, 1}, WaitMode::All, 0));
    CHECK_VALIDATION(f, "the semaphore belongs to another device", f.device->destroy(foreign));
    other.device->destroy(foreign);

    f.device->destroy(semaphore);
    SemaphoreValue destroyed = {semaphore, 1};
    CHECK_VALIDATION(f, "use of a destroyed Semaphore",
                     (void)f.device->waitSemaphores({&destroyed, 1}, WaitMode::All, 0));
    CHECK_VALIDATION(f, "use of a destroyed Semaphore", (void)semaphore->value());
    CHECK_VALIDATION(f, "use of a destroyed Semaphore", (void)semaphore->signal(9));
    CHECK_VALIDATION(f, "use of a destroyed Semaphore", (void)semaphore->userData());
    CHECK_VALIDATION(f, "use of a destroyed Semaphore", semaphore->setUserData(1));
    CHECK_VALIDATION(f, "use of a destroyed Semaphore", f.device->destroy(semaphore));
}

TEST_CASE("shutting down with a defragmentation pass open")
{
    ValidationFixture f;
    DefragmentationPass* pass = f.device->beginDefragmentation({}).value();
    REQUIRE(pass);
    CHECK_VALIDATION(f, "a defragmentation pass is still active", f.device->shutdown());
}
