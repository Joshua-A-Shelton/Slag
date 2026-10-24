#include "common/Objects.h"
#include "common/Vector.h"

#include <slag/GraphicsDevice.h>
#include <slag/util/DeferredDestroyer.h>

namespace slag {

namespace {

// In destroy order: views before their textures, resources before the memory
// blocks they're placed in, semaphores last.
enum class Kind : uint8_t
{
    AttachmentView,
    Buffer,
    Texture,
    QueryPool,
    Pipeline,
    CommandBuffer,
    MemoryBlock,
    Semaphore,
    Count,
};

struct Entry
{
    void* object;
    SemaphoreValue after;
    Kind kind;
    bool reached; // scratch, set by collect()
};

void destroyEntry(GraphicsDevice* device, const Entry& entry)
{
    switch (entry.kind)
    {
    case Kind::AttachmentView:
        device->destroy(static_cast<AttachmentView*>(entry.object));
        break;
    case Kind::Buffer:
        device->destroy(static_cast<Buffer*>(entry.object));
        break;
    case Kind::Texture:
        device->destroy(static_cast<Texture*>(entry.object));
        break;
    case Kind::QueryPool:
        device->destroy(static_cast<QueryPool*>(entry.object));
        break;
    case Kind::Pipeline:
        device->destroy(static_cast<Pipeline*>(entry.object));
        break;
    case Kind::CommandBuffer:
        device->destroy(static_cast<CommandBuffer*>(entry.object));
        break;
    case Kind::MemoryBlock:
        device->destroy(static_cast<MemoryBlock*>(entry.object));
        break;
    case Kind::Semaphore:
        device->destroy(static_cast<Semaphore*>(entry.object));
        break;
    case Kind::Count:
        break;
    }
}

} // namespace

struct DeferredDestroyer::State
{
    Vector<Entry> entries;
    Vector<SemaphoreValue> waits; // reused by flush

    // False when out of host memory.
    bool add(void* object, Kind kind, SemaphoreValue after)
    {
        return entries.pushBack({object, after, kind, false});
    }
};

DeferredDestroyer::DeferredDestroyer(GraphicsDevice* device)
    : device(device)
{
    const HostAllocator* allocator = &static_cast<GraphicsDeviceCommon*>(device)->instance->allocator;
    state = allocator->create<State>();
    if (state)
    {
        state->entries = Vector<Entry>(allocator);
        state->waits = Vector<SemaphoreValue>(allocator);
    }
}

DeferredDestroyer::~DeferredDestroyer()
{
    if (state)
    {
        flush();
        static_cast<GraphicsDeviceCommon*>(device)->instance->allocator.destroy(state);
    }
}

void DeferredDestroyer::collect()
{
    if (!state)
    {
        return;
    }
    Vector<Entry>& entries = state->entries;
    // Read every value before destroying anything: a semaphore may be both
    // destroyed and waited on in this run.
    Semaphore* lastSemaphore = nullptr;
    uint64_t lastValue = 0;
    for (Entry& entry : entries)
    {
        if (entry.after.semaphore != lastSemaphore)
        {
            lastSemaphore = entry.after.semaphore;
            lastValue = lastSemaphore->value();
        }
        entry.reached = lastValue >= entry.after.value;
    }
    for (uint8_t kind = 0; kind < static_cast<uint8_t>(Kind::Count); ++kind)
    {
        for (const Entry& entry : entries)
        {
            if (entry.reached && entry.kind == static_cast<Kind>(kind))
            {
                destroyEntry(device, entry);
            }
        }
    }
    size_t kept = 0;
    for (size_t i = 0; i < entries.size(); ++i)
    {
        if (!entries[i].reached)
        {
            entries[kept++] = entries[i];
        }
    }
    while (entries.size() > kept)
    {
        entries.popBack();
    }
}

void DeferredDestroyer::flush()
{
    if (!state || state->entries.empty())
    {
        return;
    }
    state->waits.clear();
    bool recorded = state->waits.reserve(state->entries.size());
    for (const Entry& entry : state->entries)
    {
        if (recorded)
        {
            (void)state->waits.pushBack(entry.after);
        }
        else
        {
            // Out of host memory: wait one at a time.
            (void)device->waitSemaphores({&entry.after, 1}, WaitMode::All, UINT64_MAX);
        }
    }
    if (recorded)
    {
        // A lost device completes every semaphore, so destroying still follows.
        (void)device->waitSemaphores({state->waits.data(), state->waits.size()}, WaitMode::All, UINT64_MAX);
    }
    for (uint8_t kind = 0; kind < static_cast<uint8_t>(Kind::Count); ++kind)
    {
        for (const Entry& entry : state->entries)
        {
            if (entry.kind == static_cast<Kind>(kind))
            {
                destroyEntry(device, entry);
            }
        }
    }
    state->entries.clear();
}

#define SLAG_DESTROY_AFTER(Type)                                                                                       \
    void DeferredDestroyer::destroyAfter(Type* object, SemaphoreValue after)                                           \
    {                                                                                                                  \
        if (object && (!state || !state->add(object, Kind::Type, after)))                                              \
        {                                                                                                              \
            /* Out of host memory: wait now. */                                                                        \
            (void)device->waitSemaphores({&after, 1}, WaitMode::All, UINT64_MAX);                                      \
            device->destroy(object);                                                                                   \
        }                                                                                                              \
    }
SLAG_DESTROY_AFTER(AttachmentView)
SLAG_DESTROY_AFTER(Buffer)
SLAG_DESTROY_AFTER(Texture)
SLAG_DESTROY_AFTER(QueryPool)
SLAG_DESTROY_AFTER(Pipeline)
SLAG_DESTROY_AFTER(CommandBuffer)
SLAG_DESTROY_AFTER(MemoryBlock)
SLAG_DESTROY_AFTER(Semaphore)
#undef SLAG_DESTROY_AFTER

} // namespace slag
