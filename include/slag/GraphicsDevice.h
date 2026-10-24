#pragma once

// GraphicsDevice: info, capabilities, initialization, create/destroy, descriptor
// writes, placement queries, semaphore waits, status, memory budget, and the
// pipeline cache (ADRs 0008, 0015, 0019, 0023, 0029, 0031, 0043, 0053, 0054).

#include <slag/CommandQueue.h>
#include <slag/Limits.h>
#include <slag/Result.h>
#include <slag/Semaphore.h>

#include <cstddef>
#include <cstdint>
#include <span>

namespace slag {

enum class GraphicsDeviceType : uint8_t
{
    Discrete,
    Integrated,
    Software,
};

struct GraphicsDeviceInfo
{
    char name[256] = {};
    char driverVersion[64] = {}; // for logging only; vendors encode versions differently
    uint32_t vendorId = 0;
    uint32_t deviceId = 0;
    GraphicsDeviceType type = GraphicsDeviceType::Discrete;
    uint64_t dedicatedMemoryBytes = 0;
    uint64_t sharedMemoryBytes = 0;
    uint8_t luid[8] = {}; // all zero where the platform has no LUID (Linux)
};

struct GraphicsDeviceInitDesc
{
    uint32_t resourceHeapSize = kDefaultResourceHeapSize; // up to kMaxResourceHeapSize (ADR 0029)
    uint32_t samplerHeapSize = kMaxSamplerHeapSize;
    std::span<const std::byte> pipelineCacheData; // from a previous run (ADR 0043)
    uint32_t graphicsQueueCount = 1;              // at least 1 (ADR 0054)
    uint32_t computeQueueCount = 1;
    uint32_t copyQueueCount = 1;
};

class GraphicsDevice
{
public:
    const GraphicsDeviceInfo& info() const;

    // Makes the device usable. Nothing else may use the device meanwhile.
    Result<void> initialize(const GraphicsDeviceInitDesc& desc);

    // Undoes initialize; every object created from the device must be
    // destroyed first. Nothing else may use the device meanwhile.
    void shutdown();

    // Ok, or DeviceLost once the device has been lost (ADR 0054).
    Status status() const;

    // index < the count requested for that type in initialize.
    CommandQueue* commandQueue(CommandQueueType type, uint32_t index = 0) const;

    Result<Semaphore*> createSemaphore(const SemaphoreDesc& desc);
    void destroy(Semaphore* semaphore);

    // CPU wait. Returns Timeout if the timeout passed first. Thread-safe.
    Result<void> waitSemaphores(std::span<const SemaphoreValue> values, WaitMode mode, uint64_t timeoutNs);

    // Waits for all queues. No submits or presents may run meanwhile (ADR 0068).
    Result<void> waitIdle();

    uint64_t userData() const;
    void setUserData(uint64_t value);

    GraphicsDevice(const GraphicsDevice&) = delete;
    GraphicsDevice& operator=(const GraphicsDevice&) = delete;

protected:
    GraphicsDevice() = default;
    ~GraphicsDevice() = default;
};

} // namespace slag
