# 0069 — Delayed destruction utility

**Status:** Accepted (2026-10-23). `DeferredDestroyer` is final
(docs/naming.md).

## Decision

The core library never delays destruction (`destroy()` frees immediately,
CLAUDE.md §9). An optional utility, built only on public functions like the
descriptor slot allocator (ADR 0032), handles the common case:

```cpp
// include/slag/util/DeferredDestroyer.h
class DeferredDestroyer
{
public:
    explicit DeferredDestroyer(GraphicsDevice* device);

    // Destroy `object` once `after` has been reached. One overload per object type.
    void destroyAfter(Buffer* object, SemaphoreValue after);
    void destroyAfter(Texture* object, SemaphoreValue after);
    // ... AttachmentView, Pipeline, MemoryBlock, QueryPool, CommandBuffer, Semaphore

    void collect();   // non-blocking: destroys everything whose semaphore value has been reached
    void flush();     // waits for everything pending, then destroys it (shutdown)
};
```

- Typical use: `collect()` once per frame, `flush()` at shutdown.
- One thread at a time per instance (§7); multi-threaded applications use
  one per thread.
- Uses Slag's internal containers and the instance's `AllocatorCallbacks`.
- Descriptor slots are separate: `DescriptorAllocator` already delays slot
  reuse (ADR 0032).

## Implementation (2026-10-24)

`src/util/DeferredDestroyer.cpp`, compiled into the `slag` library; the
class holds the device and a pointer to its state.

- Overloads: `AttachmentView`, `Buffer`, `Texture`, `QueryPool`, `Pipeline`,
  `CommandBuffer`, `MemoryBlock`, `Semaphore`. Null objects are ignored.
- Objects reached in one `collect()` or `flush()` are destroyed views first,
  then buffers, textures, query pools, pipelines, command buffers, memory
  blocks, and semaphores last. All values are read before anything is
  destroyed. A memory block, or a semaphore other entries wait on, is passed
  with a value no earlier than the objects that need it.
- The destructor calls `flush()`, so it runs before the device shuts down.
- Out of host memory: `destroyAfter` waits for the value and destroys at once.

## Not chosen

- No helper at all.
- Delayed destruction in the core (`destroy(object, semaphoreValue)`): hidden
  state and timing in a thin library.

## Related

Validation (ADR 0067) catches destroying objects Slag itself still
references, not objects still used by in-flight GPU work; this utility is
the intended answer to the latter.
