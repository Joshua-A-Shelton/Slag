#pragma once

// CommandQueue and SubmitDesc: submit, present, wait idle (ADRs 0053, 0054, 0057).

#include <slag/Result.h>
#include <slag/Semaphore.h>
#include <slag/Swapchain.h>

#include <cstdint>
#include <span>

namespace slag {

class CommandBuffer;
class Swapchain;

enum class CommandQueueType : uint8_t
{
    Graphics,
    Compute,
    Copy,
};

struct TimestampCalibration
{
    uint64_t gpuTimestamp = 0;   // ticks, as written by writeTimestamp
    uint64_t cpuNanoseconds = 0; // see CommandQueue::calibrateTimestamps
};

struct SubmitDesc
{
    std::span<const SemaphoreValue> waits; // before the command buffers run
    std::span<CommandBuffer* const> commandBuffers;
    std::span<const SemaphoreValue> signals;        // after they finish
    std::span<Swapchain* const> acquiredSwapchains; // ADR 0057
};

class CommandQueue
{
public:
    CommandQueueType type() const;

    // One thread at a time per queue (ADR 0068).
    Result<void> submit(const SubmitDesc& desc);

    // Shows the acquired image, which is in the Present layout. The mode is
    // one of the swapchain's switchablePresentModes (ADR 0056). Returns Ok,
    // Suboptimal or OutOfDate (reconfigure), Occluded (the window isn't
    // visible: skip rendering), or DeviceLost (ADRs 0057, 0058).
    Result<void> present(Swapchain* swapchain, PresentMode mode);

    // Timestamp ticks per second on this queue (ADR 0051).
    uint64_t timestampFrequency() const;
    // A GPU timestamp and the CPU time taken at the same moment, to put this
    // queue's timestamps on the CPU clock: nanoseconds on the platform's
    // monotonic clock (QueryPerformanceCounter on Windows, CLOCK_MONOTONIC on
    // Linux; std::chrono::steady_clock on MSVC and libstdc++). Needs
    // GraphicsDeviceCapabilities::timestampCalibration.
    Result<TimestampCalibration> calibrateTimestamps() const;

    // Waits until all work submitted to this queue has finished. One thread at
    // a time per queue (ADR 0068).
    Result<void> waitIdle();

    CommandQueue(const CommandQueue&) = delete;
    CommandQueue& operator=(const CommandQueue&) = delete;

protected:
    CommandQueue() = default;
    ~CommandQueue() = default;
};

} // namespace slag
