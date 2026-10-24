#pragma once

// Semaphore, SemaphoreDesc, SemaphoreValue (ADR 0053).

#include <slag/Result.h>

#include <cstdint>
#include <span>

namespace slag {

enum class WaitMode : uint8_t
{
    All,
    Any,
};

struct SemaphoreDesc
{
    uint64_t initialValue = 0;
    const char* debugName = nullptr;
    uint64_t userData = 0;
};

// A timeline semaphore: Slag's only synchronization primitive (ADR 0053).
class Semaphore
{
public:
    // The last value the semaphore reached. Thread-safe.
    uint64_t value() const;

    // Signals from the CPU. Values must strictly increase. One thread at a time.
    Result<void> signal(uint64_t value);
    // CPU wait until the semaphore reaches `value`. Returns Timeout if the
    // timeout passed first. Thread-safe.
    Result<void> wait(uint64_t value, uint64_t timeoutNs);

    uint64_t userData() const;
    void setUserData(uint64_t value);

    Semaphore(const Semaphore&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;

protected:
    Semaphore() = default;
    ~Semaphore() = default;
};

struct SemaphoreValue
{
    Semaphore* semaphore = nullptr;
    uint64_t value = 0;
};

// CPU wait on several semaphores of one device, for all of them or any one.
// Returns Timeout if the timeout passed first. Thread-safe.
Result<void> waitSemaphores(std::span<const SemaphoreValue> values, WaitMode mode, uint64_t timeoutNs);

} // namespace slag
