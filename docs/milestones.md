# Milestones to v1.0

**Status:** Accepted (2026-10-23).

Each milestone ends in something that runs on **both** backends and is
covered by tests. Validation (ADR 0067) grows with each milestone: whatever
a milestone adds, it also validates. Open checks from `docs/open-items.md`
are listed where a milestone settles them.

## M0 — Tooling (done)

Finish the groundwork the first real code needs.

- Fetch pinned Vulkan-Headers and the DirectX 12 Agility SDK in CMake.
- A CMake function that compiles Slang shaders with the required flags
  (ADR 0034) for tests and samples.
- Choose the unit test framework (tests only, ADR 0061) and hook it into
  CTest.
- CI skeleton: hosted build + unit tests (CLAUDE.md §12) — GitHub Actions.
- Windows side verified in CI (2026-10-24): Agility SDK fetch, header
  ordering, runtime copy, DXIL output.

**Done when:** a test shader compiles to DXIL and SPIR-V in the build, and a
trivial unit test runs in CI.

## M1 — Risk spike: compute and readback (done on RADV and WARP)

The smallest program that exercises the riskiest assumptions: a compute
shader writes a buffer through a heap descriptor, and the CPU reads it back.

- Core plumbing: `Result`/`Status`, `AllocatorCallbacks`, internal
  containers, object layout and dispatch (ADR 0065), backend registry.
- `Instance` (backend tag, debug callback, validation flags), device
  listing with baseline filtering, `GraphicsDevice::initialize`.
- Slag's Vulkan loader (ADR 0061).
- One queue, `CommandBuffer` (begin/end/reset), `Semaphore`, `submit`,
  `waitSemaphores`.
- `Buffer` with automatic placement (dedicated allocations only at first),
  `mappedData()`, readback memory.
- Resource and sampler heaps, `writeRWStructuredBuffer`, push constants.
- Compute pipeline from Slang output; `dispatch`, `barrier`, `copyBuffer`.

**Done when:** the same shader produces identical bytes on D3D12 (hardware
and WARP) and Vulkan (NVIDIA and RADV).

**Status (2026-10-23):** Vulkan side done and tested on RADV (RX 7900 XT and
Raphael iGPU): `tests/backend/ComputeTests.cpp` passes with no validation
errors.

**Status (2026-10-24):** D3D12 side done: the same test passes on WARP in
Windows CI (MSVC, debug and release) with the debug layer on. Still
unchecked: D3D12 on hardware, Vulkan on NVIDIA.

**Settles:** Slang descriptor-heap targeting and version (open items 1, 13),
descriptor sizes (19), WARP support (22), heap address-range limits (23),
plus the driver-reserved heap range in practice.

## M2 — Resources and memory

- All seven texture types, `AttachmentView`, castable formats.
- `MemoryBlock`, placed resources, aliasing.
- The shared sub-allocator, with the per-block bookkeeping defragmentation
  needs later (ADR 0062).
- All memory kinds, mapping rules, memory budget.
- All copies, `copyFootprint`, `placementInfo`.
- `PixelFormat` tables, `formatSupport()`, every baseline check including the
  ◐ format entries; limits constants.
- Device info and capabilities.

**Done when:** every resource type can be created, filled, copied, and read
back on both backends, with tests for each.

**Settles:** ◐ format entries (16), resource heap tier 2 coverage (15).

## M3 — Off-screen graphics

- Graphics pipelines with all state (ADRs 0037–0042), vertex input with
  SPIR-V input reflection (ADR 0038), the root constant buffer slot.
- Render passes (including split passes), draws, indexed and indirect
  draws with draw IDs.
- Samplers and all descriptor kinds.
- An image comparison harness: render the same scene on both backends and
  compare.

**Done when:** a textured, depth-tested scene renders off-screen and
matches across backends.

**Settles:** render-pass resolve layout (10), mixed depth/stencil layout (11).

## M4 — Presentation

- `Swapchain` for Win32, Xlib, XCB, Wayland; acquire and present;
  `reconfigure`; present modes and switchable modes; HDR color spaces and
  metadata; frame latency.
- Choose the windowing library for samples (ADR 0061).
- Samples: `triangle`, `textured_quad`.

**Done when:** both samples run in a window on Windows (both backends) and
Linux (X11 and Wayland), surviving resize and minimize.

## M5 — Queues, synchronization, utilities

- Queue counts per type, compute and copy queues, shared-queue handling.
- Pipeline cache blob (with hashed D3D12 keys).
- `DescriptorAllocator` and `DeferredDestroyer` utilities.
- Samples: `compute`, `bindless`, `multi_queue`.

**Done when:** the multi-queue sample overlaps compute and copy work with
graphics on both backends.

## M6 — Defragmentation

- Defragmentation passes as designed in ADR 0062: `movable` placement flag,
  `beginDefragmentation` / move list / `recordCopies` /
  `endDefragmentation`, stable pointers, Slag-rebuilt attachment views.
- Validation: destroying a resource in an active pass, stale descriptor
  slots after a pass, the semaphore value given to `endDefragmentation`.
- A test that fragments memory, runs passes, and checks that moved buffers
  and textures keep their contents on both backends.

**Done when:** a pass frees whole memory blocks after a fragmenting
allocation pattern, and every moved resource reads back unchanged on both
backends.

## M7 — Debugging and diagnostics

- Stale-pointer quarantine, descriptor-slot tracking, D3D12 poison
  descriptors.
- Debug names, labels, queries (timestamps, occlusion), crash diagnostics.
- Validation tests for every check.

**Done when:** every check in ADR 0067 has a test that triggers it, and a
deliberately crashing sample produces a crash report on both backends.

**Settles:** GPU-based validation coverage (12).

## M8 — v1.0

- CI complete: hosted WARP + self-hosted RADV runner (CLAUDE.md §12).
- Remaining open items closed or explicitly deferred.
- API documentation pass, `CHANGELOG.md`, tag `v1.0.0`.

## After v1.0

Mesh shaders and ray tracing (ADR 0005),
sparse resources (0026), external memory and OpenXR (0027), native device
import (0018), transparent windows (0056), the Vulkan 1.3 fallback
evaluation (CLAUDE.md §3), ARM64 support (0070).
