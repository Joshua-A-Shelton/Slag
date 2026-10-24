# 0062 — Defragmentation

**Status:** Accepted (2026-10-23). Implemented before v1.0 (milestone M6,
`docs/milestones.md`); the allocator built in M2 must already satisfy the
constraints below. Names are placeholders.

## Goal

Compact automatically placed resources into fewer memory blocks, so freed
space can be returned and large allocations still fit after long runs.

## Constraints on the allocator (so defragmentation fits without API breaks)

1. **Stable pointers.** A moved resource keeps its `Buffer*` / `Texture*`;
   Slag swaps the native object behind it. (Opaque pointers, ADR 0052, make
   this possible.)
2. **Only flagged, automatically placed resources move.** Automatic placement
   has an opt-in `movable` flag (default off); only flagged resources ever
   appear in a move list. Resources in a user's `MemoryBlock` and dedicated
   allocations never move.

   ```cpp
   .placement = Placement::automatic(MemoryKind::GpuOnly, { .movable = true }),
   ```
3. **The allocator keeps per-block bookkeeping** of every allocation (owner,
   offset, size, alignment), so a pass can plan moves.
4. **Nothing moves implicitly.** Moves happen only inside an explicit,
   user-driven pass.

## The pass

```cpp
DefragmentationPass* pass = device->beginDefragmentation(desc);   // budget: max bytes / moves
std::span<const DefragmentationMove> moves = pass->moves();       // resource, old/new location
pass->recordCopies(commandBuffer);   // Slag records the copies
// user submits, and keeps every moved resource unused by other GPU work until it completes
device->endDefragmentation(pass, /*completed*/ semaphoreValue);   // swaps native objects, frees old space
```

- Like ADR 0058's `reconfigure`, nothing waits internally: the user
  guarantees the GPU is done (semaphore value), validated in validation
  builds.
- After a move, **descriptors and GPU addresses of moved resources are
  stale**. The user rewrites them from the move list: since pointers are
  stable, repeating the original descriptor write works unchanged. Slag
  does not track or rewrite slots (it tracks them only in validation builds,
  where it reports slots still pointing at a moved resource's old location).
- Textures are copied in their current layout; the pass's move list states
  the layout the user must have them in (`CopySource` / after the copy the
  same layout as before).
- Mapped pointers of moved buffers change; the move list flags them.
- **Attachment views of moved textures are rebuilt by Slag** in
  `endDefragmentation` (they are Slag objects); `AttachmentView*` pointers
  keep working. So the user only fixes descriptor slots, GPU addresses, and
  mapped pointers.
- `userData` (ADR 0063) gets from a moved resource back to its owner.

## Decided

- Opt-in `movable` flag, not movable-by-default with per-move vetoes.
- The user rewrites descriptor slots; Slag doesn't remember descriptor
  writes for this.
