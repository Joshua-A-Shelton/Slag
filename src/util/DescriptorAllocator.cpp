#include "common/Logging.h"
#include "common/Objects.h"
#include "common/Vector.h"

#include <slag/util/DescriptorAllocator.h>

namespace slag {

struct DescriptorAllocator::State
{
    GraphicsDeviceCommon* device = nullptr;
    uint32_t firstSlot = 0;
    uint32_t endSlot = 0;  // one past the range
    uint32_t nextSlot = 0; // first slot never handed out
    Vector<uint32_t> freeSlots;
    struct Pending
    {
        uint32_t slot;
        SemaphoreValue after;
    };
    Vector<Pending> pending;
};

DescriptorAllocator::DescriptorAllocator(GraphicsDevice* device, const DescriptorAllocatorDesc& desc)
{
    auto* common = static_cast<GraphicsDeviceCommon*>(device);
    [[maybe_unused]] uint32_t heapSize =
        desc.heap == DescriptorHeap::Resource ? common->resourceHeapSize : common->samplerHeapSize;
    SLAG_VALIDATE(common->instance, common->initialized, "DescriptorAllocator", "the device is not initialized");
    SLAG_VALIDATE(common->instance,
                  desc.slotCount > 0 && desc.firstSlot <= heapSize && desc.slotCount <= heapSize - desc.firstSlot,
                  "DescriptorAllocator", "slots %u to %u are outside the %u-slot heap", desc.firstSlot,
                  desc.firstSlot + desc.slotCount, heapSize);
    const HostAllocator* allocator = &common->instance->allocator;
    state = allocator->create<State>();
    if (!state)
    {
        return;
    }
    state->device = common;
    state->firstSlot = desc.firstSlot == 0 ? 1 : desc.firstSlot;
    state->endSlot = desc.firstSlot + desc.slotCount;
    state->nextSlot = state->firstSlot;
    state->freeSlots = Vector<uint32_t>(allocator);
    state->pending = Vector<State::Pending>(allocator);
}

DescriptorAllocator::~DescriptorAllocator()
{
    if (state)
    {
        state->device->instance->allocator.destroy(state);
    }
}

uint32_t DescriptorAllocator::allocate()
{
    if (!state)
    {
        return 0;
    }
    if (!state->freeSlots.empty())
    {
        uint32_t slot = state->freeSlots[state->freeSlots.size() - 1];
        state->freeSlots.popBack();
        return slot;
    }
    return state->nextSlot < state->endSlot ? state->nextSlot++ : 0;
}

void DescriptorAllocator::free(uint32_t slot, SemaphoreValue after)
{
    if (!state)
    {
        return;
    }
    [[maybe_unused]] InstanceCommon* instance = state->device->instance;
    SLAG_VALIDATE(instance, slot >= state->firstSlot && slot < state->nextSlot, "DescriptorAllocator::free",
                  "slot %u was not handed out by this allocator", slot);
    SLAG_VALIDATE(instance, after.semaphore, "DescriptorAllocator::free", "the semaphore is null");
    if (!state->pending.pushBack({slot, after}))
    {
        logMessage(messageSink(instance), MessageSeverity::Warning, MessageSource::Slag,
                   "slag: DescriptorAllocator::free: out of host memory; slot %u is not reused", slot);
    }
}

void DescriptorAllocator::collect()
{
    if (!state)
    {
        return;
    }
    // Most pending slots share a semaphore; read each one's value once per run.
    Semaphore* lastSemaphore = nullptr;
    uint64_t lastValue = 0;
    size_t kept = 0;
    for (size_t i = 0; i < state->pending.size(); ++i)
    {
        State::Pending entry = state->pending[i];
        if (entry.after.semaphore != lastSemaphore)
        {
            lastSemaphore = entry.after.semaphore;
            lastValue = lastSemaphore->value();
        }
        // The free list never outgrows the range, so its growth failing only
        // delays the slot to a later collect.
        if (lastValue >= entry.after.value && state->freeSlots.pushBack(entry.slot))
        {
            continue;
        }
        state->pending[kept++] = entry;
    }
    while (state->pending.size() > kept)
    {
        state->pending.popBack();
    }
}

} // namespace slag
