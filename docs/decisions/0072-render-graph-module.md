# 0072 — Render graph module

**Status:** Accepted (2026-10-24). Names are final (docs/naming.md;
settled after M9).

## Decided

- **Separate, optional module**, not part of the `slag` library: its own
  target (`slag::rendergraph`), headers in `include/slag/rendergraph/`,
  sources in `src/rendergraph/`, and a CMake option. It is built only on
  public Slag functions, like `util/`. The core keeps CLAUDE.md §2: no render
  graph, no barrier inference, no state tracking.
- **Built before `v1.0.0` is tagged** (milestone M9). It works the explicit
  API hardest where breaks would hide (barrier state across submits,
  multithreaded recording, defragmentation), so whatever it shows can still
  change the core without a major version.
- **Scope of the first version:** barriers and layouts worked out from
  declared reads and writes; parallel recording of passes; transient
  resources aliased in shared memory; passes on several queues, with the
  semaphores between them.
- **Exempt from ADR 0007:** the module's API may use lambdas and templates.
  It will never have a C API. CLAUDE.md §9 still holds: its CPU memory goes
  through `AllocatorCallbacks`, so callables are stored in memory the graph
  allocates, not in `std::function`.

## Design (accepted 2026-10-24)

- **Declaration order is execution order.** The graph works out barriers,
  semaphores, and memory aliasing; it never reorders or culls passes.
- **State per whole resource** in the first version; per-mip tracking (mip
  generation) later.
- **The graph begins and ends rendering** for passes that declare
  attachments, with their load and store ops.
- **One command buffer per pass**; grouping passes into one command buffer
  later.
- **No thread pool:** `recordPass(i)` may be called from any thread for
  different passes; the user's job system runs them.
- **Pass code is stored through templates** in memory the graph allocates
  (`AllocatorCallbacks`), not in `std::function`.
- **Passes refer to resources through small handles**, since transient
  resources don't exist until `compile`.
- **Rebuilt every frame**; caching `compile` results comes later.
- Barriers for a pass are recorded at the start of its command buffer, so
  every barrier is known before recording starts and passes record in
  parallel. One timeline semaphore per queue; a dependency between queues
  splits submits and adds a wait. Textures used on a copy queue are moved to
  `Common` on the queue they come from; compute queues get only the layouts
  D3D12 allows there.
- Transient resources live in memory blocks the graph owns, placed by
  lifetime and reused when lifetimes don't overlap; a placed resource's first
  use starts from `Undefined`, after a barrier from the previous resource's
  last use. Their `Texture` / `Buffer` objects are cached across frames by
  description and placement.

## Core prerequisites (done 2026-10-24)

- Barrier ordering across submits on Vulkan (ADR 0049, "Submit
  boundaries").
- Per-queue layout validation (ADR 0049).

## Implementation (M9, 2026-10-24)

- **Planner** (`src/rendergraph/Plan.cpp`, plain data, unit-tested without
  a GPU): batches, waits, barriers, handoffs, final layouts, placement.
- **Submits:** consecutive passes of a queue share a submit until one uses a
  resource last used on another queue; that closes both queues' open
  submits, so every wait names a submit made earlier (no cycles, no wait
  before its signal). Any change of queue between two uses of a resource is
  ordered by a semaphore, even read after read, so a resource's uses are
  totally ordered. Each submit signals the graph's semaphore for its queue.
- **Barriers:** within a submit, from the earlier accesses (reads after a
  write share one barrier until a new stage or access needs visibility);
  across submits, only layout changes, from `syncBefore = None` (ADR 0049).
  A layout the next queue can't use is changed at the end of the last use,
  on its own queue, to the next layout if that queue allows it, else
  `Common`.
- **Imported textures** start in their initial layout with earlier work
  finished (the graph's first use is a later submit) and end in their final
  layout: at the end of the last use, or in an epilogue submit on the
  graphics queue when that queue can't make the change (compute and copy
  queues can't make `Present`). An acquired swapchain image's submit is the
  one that first uses it.
- **Transients:** placed largest first at the lowest offset clear of every
  transient whose uses aren't ordered against its own (same submit, or a
  wait chain). Taking over memory in the same submit: the old resource gets
  a barrier from its accesses to `SyncAfter = None` / `NO_ACCESS` (D3D12
  allows `NO_ACCESS` only with `SyncAfter = None` or an `Undefined` layout
  after, which Vulkan forbids), the new one a barrier from `Undefined` with
  `syncBefore` covering the old accesses and `NO_ACCESS` before (the
  Enhanced Barriers aliasing steps). Across submits, nothing but `Undefined`.
  Memory is one `GpuOnly` block per frame in flight, grown as needed;
  `Texture` / `Buffer` objects are reused across frames by description and
  offset.
- **Checks** (reported through `messageCallback`; `compile` returns
  `Unsupported`): layouts per queue, usage of transient textures, a
  transient read before written or loaded on first use, one layout per
  texture per pass (sampling a read-only depth attachment shares
  `DepthStencilRead`), a texture first used on a copy queue must be imported
  in `Common`, imported attachments need views.
- **Sync per access:** shader accesses use `AllShading` on graphics passes
  and `ComputeShading` on compute passes; vertex buffers `IndexInput |
  VertexShading`, index buffers `IndexInput`, indirect arguments
  `ExecuteIndirect`.
- Tests: `RenderGraphPlanTests.cpp` (unit), `RenderGraphTests.cpp` (every
  backend, synchronization validation on), and `samples/render_graph`. Run
  on RADV, and on the RTX 2070 SUPER with D3D12 and Vulkan.
- The public header also includes `<new>`, `<type_traits>`, and `<utility>`
  (non-allocating), beyond ADR 0066's list, for storing lambdas.
