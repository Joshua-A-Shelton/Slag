#pragma once

// CommandQueue and SubmitDesc: submit, present, wait idle (ADRs 0053, 0054, 0057).

#include <slag/Result.h>

#include <cstdint>

namespace slag {

enum class CommandQueueType : uint8_t
{
    Graphics,
    Compute,
    Copy,
};

class CommandQueue
{
public:
    CommandQueueType type() const;

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
