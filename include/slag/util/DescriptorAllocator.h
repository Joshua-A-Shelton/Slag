#pragma once

// Optional descriptor slot allocator utility (ADR 0032). Built only on public Slag functions.

#include <slag/Semaphore.h>

#include <cstdint>

namespace slag {

class GraphicsDevice;

enum class DescriptorHeap : uint8_t
{
    Resource,
    Sampler,
};

struct DescriptorAllocatorDesc
{
    DescriptorHeap heap = DescriptorHeap::Resource;
    // The range handed out; the rest of the heap stays the caller's. Slot 0 is
    // never handed out, so a range starting at 0 starts at 1.
    uint32_t firstSlot = 1;
    uint32_t slotCount = 0;
};

// Hands out single slots in a range of one heap. A freed slot is reused only
// once its semaphore value is reached. Not thread-safe (CLAUDE.md §7): one
// thread at a time, or one allocator per thread with separate ranges.
class DescriptorAllocator
{
public:
    DescriptorAllocator(GraphicsDevice* device, const DescriptorAllocatorDesc& desc);
    ~DescriptorAllocator();

    // 0 when the range is exhausted (or out of host memory); collect() may
    // return freed slots.
    uint32_t allocate();
    // The slot may be handed out again once `after` is reached. A slot whose
    // free can't be recorded (out of host memory) is not reused.
    void free(uint32_t slot, SemaphoreValue after);
    // Returns slots whose semaphore value has been reached to the free list.
    void collect();

    DescriptorAllocator(const DescriptorAllocator&) = delete;
    DescriptorAllocator& operator=(const DescriptorAllocator&) = delete;

private:
    struct State;
    State* state = nullptr;
};

} // namespace slag
