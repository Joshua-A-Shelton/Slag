#pragma once

// Stale-pointer quarantine (ADR 0052), validation builds: destroyed objects
// keep their memory, marked destroyed, so later use can be reported. The
// last `capacity` destroyed objects of all types stay; older ones are freed.
// Thread-safe (ADR 0068).

#include <cstddef>
#include <mutex>

namespace slag {

struct InstanceCommon;

struct Quarantine
{
    static constexpr size_t capacity = 4096;

    using ReleaseFn = void (*)(InstanceCommon* instance, void* object);
    struct Entry
    {
        void* object = nullptr;
        ReleaseFn release = nullptr;
    };

    // Frees the oldest entry when full.
    void add(InstanceCommon* instance, void* object, ReleaseFn release);
    // Frees every entry (instance destruction).
    void flush(InstanceCommon* instance);

    Entry entries[capacity];
    size_t next = 0;
    std::mutex lock;
};

} // namespace slag
