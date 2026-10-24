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

## Not chosen

- No helper at all.
- Delayed destruction in the core (`destroy(object, semaphoreValue)`): hidden
  state and timing in a thin library.

## Related

Validation (ADR 0067) catches destroying objects Slag itself still
references, not objects still used by in-flight GPU work; this utility is
the intended answer to the latter.
