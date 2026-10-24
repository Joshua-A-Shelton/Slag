# 0063 — User data on objects

**Status:** Accepted (2026-10-23). Names final (docs/naming.md).

## Decision

- Every `Desc` has an optional `uint64_t userData` (default 0), and every
  Slag object has `userData()` and `setUserData(uint64_t)`.
- Slag never reads or interprets it.
- `uint64_t`, not `void*`: holds a pointer or an index directly, independent
  of pointer size (C-shaped API, ADR 0007).
- `setUserData` is externally synchronized like other object calls (§7).

## Why

Gets from a Slag object back to the user's owner (material, asset, mesh)
without a lookup table: mainly for defragmentation moves (ADR 0062), also for
debug tooling and the user's own reports.

```cpp
Texture* tex = device->createTexture2D({
    // ...
    .placement = Placement::automatic(MemoryKind::GpuOnly, { .movable = true }),
    .userData = reinterpret_cast<uint64_t>(material),
});

for (const DefragmentationMove& m : pass->moves()) {
    auto* owner = reinterpret_cast<Material*>(m.texture->userData());
    owner->rewriteDescriptors(device);
}
```
