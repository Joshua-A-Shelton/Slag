# 0072 — Render graph module

**Status:** Accepted in part (2026-10-24): placement, timing, scope, and the
ADR 0007 exemption are decided; the design is proposed and still open. Names
are placeholders.

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

## Open

The design (pass declaration, resource handles, ordering, queue scheduling,
transient aliasing, recording model, frame lifetime, defragmentation
support), and two core questions the graph depends on: barrier ordering
across submits on Vulkan, and per-queue layout validation (ADR 0049).
