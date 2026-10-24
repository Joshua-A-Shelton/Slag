# 0062 — Defragmentation

**Status:** Accepted (2026-10-23). Implemented before v1.0 (milestone M6,
`docs/milestones.md`); the allocator built in M2 must already satisfy the
constraints below. Names are final (docs/naming.md; settled after M6).

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

## Decided before M6 (2026-10-24)

- **Movable kinds:** `GpuOnly` and `GpuUpload`. `movable` on Upload or
  Readback is a validation error (and ignored). Moved GpuUpload buffers get
  a new mapped pointer: `mappedData()` returns it after `endDefragmentation`.
  Textures only live in GpuOnly memory.
- **Textures are copy-ready, on any queue.** Before `recordCopies`, moved
  textures are in `CopySource` (graphics or compute command buffer) or
  `Common` (copy queue, ADR 0049), and moved buffers' earlier writes are
  visible to copies. Slag initializes each new texture, copies, and leaves it
  in that same layout with no access pending: the user's next barrier on a
  moved resource starts from `syncBefore = None`. Until
  `endDefragmentation` swaps the native objects, `Buffer*` and `Texture*`
  still refer to the old copies, so barriers on them belong after the pass
  (found on WARP, 2026-10-24). D3D12 creates the new
  texture in `COMMON` and moves it to `COPY_DEST` on graphics/compute queues
  (a full-resource copy initializes placed render targets); Vulkan moves it
  from `UNDEFINED` to `TRANSFER_DST_OPTIMAL`, or `GENERAL` on copy queues
  (Vulkan allows image barriers on transfer queues).
- **Budget:** `DefragmentationDesc { kind, maxBytes, maxMoves }`, 0 meaning
  no limit. One kind per pass.
- **Stale descriptor slots** (validation builds): every buffer and texture
  descriptor write records the resource in a per-slot table. After
  `endDefragmentation`, the next submit reports each slot still holding a
  moved resource's old descriptor, once. Constant buffer descriptors are
  written by address, so they aren't tracked; neither are GPU addresses.

## Implementation (M6)

- **Planning** is in the sub-allocator, under the kind's pool lock. Source
  blocks are those holding only movable allocations, emptiest first; each
  is evacuated into the fullest other non-empty blocks, never into a block
  being evacuated. A source is taken only if all of it fits the budget and
  the free space, so passes free whole blocks. When no whole block fits the
  budget, the emptiest one is evacuated partially, so repeated passes still
  make progress. Blocks being evacuated take no new allocations during the
  pass. Freed blocks follow the usual rule (one empty block kept per kind).
- **Moves:** `beginDefragmentation` reserves the new ranges and creates a
  native object for each move placed there. `endDefragmentation` swaps the
  native handles into the user's object (pointers stay stable), destroys
  the old ones, frees the old ranges, and rebuilds the texture's attachment
  views the same way. Moved resources lose their debug names until names
  are kept (M7).
- One pass per device at a time. Destroying a resource in an active pass is
  a validation error; Slag then drops its move.
- `endDefragmentation(pass, completed)` checks (validation builds) that
  `recordCopies` was called when there are moves and that `completed` has
  been reached. It waits for nothing. Other queues that use moved resources
  afterwards should wait on the same value, which makes the copies visible
  to them.
