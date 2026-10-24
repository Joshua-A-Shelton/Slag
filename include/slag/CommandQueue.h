#pragma once

// CommandQueue and SubmitDesc: submit, present, wait idle (ADRs 0053, 0054, 0057).

#include <slag/Result.h>
#include <slag/Semaphore.h>

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
