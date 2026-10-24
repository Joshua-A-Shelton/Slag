# 0072 — Render graph module

**Status:** Accepted (2026-10-24). Names are placeholders.

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

## Core prerequisites

- Barrier ordering across submits on Vulkan (ADR 0049).
- Per-queue layout validation (ADR 0049).
