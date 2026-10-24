# 0066 — CPU allocator and containers

**Status:** Accepted (2026-10-23). `AllocatorCallbacks` is final
(docs/naming.md); container names are internal.

CPU-side memory only. GPU memory uses Slag's own sub-allocator (ADR 0061).

## Allocator interface

```cpp
struct AllocatorCallbacks
{
    void* (*allocate)(size_t size, size_t alignment, void* userData);
    void (*free)(void* pointer, size_t size, void* userData);
    void* userData;
};
// InstanceDesc::allocator; null callbacks = aligned malloc/free
```

- Passed in `InstanceDesc`; used for everything created from that instance.
- No `reallocate`: containers allocate, move, then free.
- Callbacks must be thread-safe; Slag may call them concurrently.
- `allocate` returning null makes the triggering call return
  `OutOfHostMemory`. Slag never aborts on allocation failure.
- Covers Slag's own allocations only. Driver allocations are not routed
  through it on either backend (D3D12 has no hook; Vulkan's
  `VkAllocationCallbacks` is not used), which keeps the backends consistent.

## Internal containers (`src/common/`)

| Container | Used for |
|---|---|
| `Vector<T>` | growable arrays |
| `SmallVector<T, N>` | inline storage for short per-call lists (barrier batches) |
| `HashMap<K, V>` | open addressing (pipeline-cache index, sampler deduplication) |
| `ObjectPool<T>` | per-object-type storage for Slag objects; also holds the stale-pointer quarantine (ADR 0052) |

- All use the allocator callbacks.
- Operations that may grow are `[[nodiscard]]` and return `false` on
  allocation failure; callers propagate `OutOfHostMemory`.
- Object pools take a lock on create/destroy (§7); lookups don't.
- Kept deliberately small: no iterator debugging, no allocator templates.

## Standard library

- **`src/`:** non-allocating headers only — `<cstdint>`, `<cstddef>`,
  `<cstring>`, `<span>`, `<string_view>`, `<type_traits>`, `<utility>`,
  `<new>`, `<bit>`, `<atomic>`, `<mutex>`, `<algorithm>`, plus the C
  headers `<cstdio>` (formatting only), `<cstdarg>`, `<cstdlib>`, `<csignal>`. No `std::vector`,
  `unordered_map`, `string`, `function`, `shared_ptr`, or iostreams. A failed
  `std::mutex` lock terminates (exceptions are off); acceptable, since it
  only happens when the OS is out of resources.
- **Public headers:** `<cstdint>`, `<cstddef>`, `<span>`, `<string_view>`.
- **Tests and samples:** the full standard library.

## Recording never allocates after warm-up

Command buffers keep their internal arrays (validation state, D3D12 barrier
batches) across `reset()`, so recording does no allocation once capacities
have settled.
