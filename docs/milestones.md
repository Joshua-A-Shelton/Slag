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
Windows CI (MSVC, debug and release) with the debug layer on. Also passes
on NVIDIA (RTX 2070 SUPER, Windows 11) on both D3D12 and Vulkan.

**Settles:** Slang descriptor-heap targeting and version (open items 1, 13),
descriptor sizes (19), WARP support (22), heap address-range limits (23),
plus the driver-reserved heap range in practice.

## M2 — Resources and memory

- All seven texture types, `AttachmentView`, castable formats.
- `MemoryBlock`, placed resources, aliasing.
- The shared sub-allocator, with the per-block bookkeeping defragmentation
  needs later (ADR 0062).
- All memory kinds, mapping rules, memory budget.
- All copies except `resolveTexture` (moved to M3: MSAA content needs
  rendering), `copyFootprint`, `placementInfo`.
- `PixelFormat` tables, `formatSupport()`, every baseline check including the
  ◐ format entries; limits constants.
- Device info and capabilities.

**Done when:** every resource type can be created, filled, copied, and read
back on both backends, with tests for each.

**Settles:** ◐ format entries (16), resource heap tier 2 coverage (15).

**Status (2026-10-24):** implemented on both backends: formats and the
format baseline, capabilities, memory budget, memory blocks, placed and
sub-allocated (TLSF) resources, all seven texture types, attachment views,
texture barriers, copies, and `copyFootprint`. Every texture type
round-trips buffer → texture → texture → buffer byte for byte on RADV (RX
7900 XT, Raphael iGPU) and WARP. RADV and WARP meet every ◐ entry. Names
proposed during M2 were reviewed and settled (`docs/naming.md`).

## M3 — Off-screen graphics

- Graphics pipelines with all state (ADRs 0037–0042), vertex input with
  SPIR-V input reflection (ADR 0038), the root constant buffer slot.
- Render passes (including split passes), draws, indexed and indirect
  draws with draw IDs.
- `resolveTexture` (moved from M2), tested on rendered MSAA targets.
- Samplers and all descriptor kinds.
- An image comparison harness (decided 2026-10-24). No machine or CI job
  runs both backends together yet, so each backend is checked on its own:
  - pixels with a known value (flat colors, depth-tested overlaps,
    point-sampled texels) are checked exactly against values the test
    computes;
  - the whole image is compared with a committed golden image (raw RGBA8,
    `tests/golden/`) within a tolerance (channel difference ≤ 2, at most
    0.5% of pixels off), covering triangle edges;
  - when both backends are available, the harness also compares them
    directly.
  On failure the harness writes the actual image next to the golden one;
  `SLAG_UPDATE_GOLDEN=1` regenerates goldens.

**Done when:** a textured, depth-tested scene renders off-screen and
passes the exact and golden checks on both backends.

**Settles:** render-pass resolve layout (10), mixed depth/stencil layout (11).

**Status (2026-10-24):** implemented on both backends: every descriptor kind
and samplers, graphics pipelines, render passes (clears, resolves,
read-only and mixed depth/stencil, split passes), draw state, direct and
indirect draws with draw IDs, dispatchIndirect, the root constant buffer,
and resolveTexture. The textured, depth-tested scene passes the exact check
and its golden on RADV and WARP (golden recorded on RADV). Changed on the way: vertex attributes are matched
by location (ADR 0038); Vulkan now requires `shaderDrawParameters` and
`separateDepthStencilLayouts` (ADR 0012). Names proposed during M3 were
reviewed and settled (`docs/naming.md`).

## M4 — Presentation

- `Swapchain` for Win32, Xlib, XCB, Wayland; acquire and present;
  `reconfigure`; present modes and switchable modes; HDR color spaces and
  metadata; frame latency.
- Windowing library for samples: SDL3 (decided 2026-10-24, ADR 0061);
  sample textures are generated in code.
- Samples: `triangle`, `textured_quad`. Each takes `--frames N`: render N
  frames, resize the window once, and exit non-zero on any error or
  validation message. They run here on Wayland and X11, and in Windows CI
  on WARP (D3D12); the project owner checks them by eye, including Vulkan
  on Windows.

**Done when:** both samples run in a window on Windows (both backends) and
Linux (X11 and Wayland), surviving resize and minimize.

**Status (2026-10-24):** swapchains on both backends (Win32, Xlib, XCB,
Wayland), with present modes, reconfigure, Occluded, HDR color spaces and
metadata, and frame latency. Both samples pass `--frames 120` (including
a resize) on RADV under Wayland, Xlib, and XCB, and `--frames 300` on an
NVIDIA RTX 2070 SUPER under Windows 11 with D3D12 and Vulkan, with no
validation messages. The whole backend suite (59 tests, including the
direct D3D12-vs-Vulkan scene comparison) passes there too. Still to check:
minimize by hand. Names proposed during M4 were reviewed and settled (`docs/naming.md`).

## M5 — Queues, synchronization, utilities

- Queue counts per type, compute and copy queues, shared-queue handling.
- Pipeline cache blob (with hashed D3D12 keys).
- `DescriptorAllocator` and `DeferredDestroyer` utilities.
- Timestamp queries (moved from M7, decided 2026-10-24): `QueryPool`,
  `writeTimestamp`, `resolveQueries`, timestamp frequency, CPU/GPU clock
  calibration, so the multi-queue sample can measure overlap.
- Samples: `compute` (a compute shader animates a RWTexture drawn
  full-screen), `bindless` (about 1000 quads, each picking its texture by
  index, with textures streamed in and out through `DescriptorAllocator`
  and `DeferredDestroyer`), `multi_queue` (particles on the compute queue,
  texture uploads on the copy queue, rendering on the graphics queue,
  chained by semaphores; prints how much compute and copy work overlapped
  graphics work, from timestamps).

**Done when:** the multi-queue sample overlaps compute and copy work with
graphics on both backends.

**Status (2026-10-24):** implemented on both backends: pipeline cache blob,
timestamp queries with calibration, `DescriptorAllocator` and
`DeferredDestroyer`, queue tests on both backends (including a
compute → copy → graphics semaphore chain), and the three samples. The
multi-queue sample draws its particles in a submit of their own (so the
pass doesn't wait for the swapchain image) and presents with Mailbox or
Immediate where it can. Overlap with graphics work, measured by the sample:

| Device | Compute | Copy |
|---|---|---|
| RX 7900 XT, RADV (Vulkan) | 64–73% | 69–79% |
| RTX 2070 SUPER, D3D12 | 47% | 48% |
| RTX 2070 SUPER, Vulkan | 75% | not measured: NVIDIA's transfer family can't resolve queries (ADR 0051) |

All five samples pass `--frames 300` on the NVIDIA machine with both
backends, and `--frames 120` on RADV, with no validation messages. Names
proposed during M5 were reviewed and settled (`docs/naming.md`).

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

**Status (2026-10-24):** implemented on both backends, as decided in ADR
0062. Movable kinds are GpuOnly and GpuUpload. Copies run on any queue,
with textures copy-ready. The budget is a kind, `maxBytes` and `maxMoves`.
Stale slots are reported at the next submit. The test:
- allocates 1 GB of 1 MB buffers and 32 textures, all movable and stamped;
- frees three quarters of the buffers and half the textures;
- runs passes until nothing moves (the first limited to 64 moves,
  alternating graphics and copy queues).

Results:

| Device | Passes | Moved | Memory in use |
|---|---|---|---|
| RX 7900 XT, RADV (Vulkan) | 2 | 128 buffers, 8 textures | 1552 → 1041 MB |
| RTX 2070 SUPER, D3D12 | 2 | 128 buffers, 8 textures | 1573 → 1061 MB |
| RTX 2070 SUPER, Vulkan | 2 | 128 buffers, 8 textures | 1610 → 1098 MB |
| WARP (hosted CI), D3D12 | 2 | 191 buffers, 1 texture | 1284 → 520 MB |

Every moved buffer and texture (all mips) reads back unchanged. A moved
render target renders through its rebuilt attachment view. The three
validation checks have tests. Names proposed during M6 were reviewed and
settled (`docs/naming.md`).

## M7 — Debugging and diagnostics

- Stale-pointer quarantine, descriptor-slot tracking, D3D12 poison
  descriptors.
- Debug names, labels, occlusion queries (timestamps moved to M5), crash
  diagnostics.
- Validation tests for every check.

**Done when:** every check in ADR 0067 has a test that triggers it, and a
deliberately crashing sample produces a crash report on both backends.

**Settles:** GPU-based validation coverage (12).

**Status (2026-10-24):** done on both backends.

- **Validation tests:** all 398 checks in `src/` fire in a test, without a
  GPU, on a null test backend (`tests/validation/`, ADR 0067);
  `validation_coverage` keeps it that way.
- **Crash sample:** `samples/crash` loses the device on purpose (it is run by
  hand, never by CTest). On the RTX 2070 SUPER:
  - Vulkan (`VK_EXT_device_fault`) reports the faulting write and
    instruction addresses.
  - D3D12 (DRED) reports a hang with the named command list, its queue, and
    its last operations. NVIDIA gave DRED no page-fault data (ADR 0014).
- **Also landed:**
  - Debug names kept in every build.
  - The 4096-object stale-pointer quarantine, and "use of a destroyed"
    checks on every call.
  - Labels.
  - Occlusion queries (exact counts, matching the pixels drawn).
  - Descriptor-slot tracking with D3D12 poison descriptors.
- **Settled open items:** 12 (D3D12 GPU-based validation reports
  never-written descriptor reads) and 19 (NVIDIA descriptor sizes).
- **Changed:**
  - Render-pass resolves were cut: the RenderTarget layout for the
    destination was wrong on D3D12 (ADR 0049).
  - Calls now return after reporting where going on would corrupt Slag's own
    state (ADR 0067).
- **Fixed:** `resolveTexture` never rejected integer or depth formats.
- **Not built:** the "resetting an in-flight command buffer" check (ADR 0067).
- **Runs:** every sample passes on RADV and, with both backends, on NVIDIA;
  hosted CI is green.
- **Names:** proposed during M7 were reviewed and settled (`docs/naming.md`).

## M8 — v1.0

- CI complete: hosted CI with WARP. No self-hosted RADV runner (decided
  2026-10-24): Vulkan runs by hand on RADV and NVIDIA before each release
  (CLAUDE.md §12).
- Native interop accessors (ADR 0071), added to M8 on 2026-10-24.
- Remaining open items closed or explicitly deferred.
- API documentation pass (README and header comments), `CHANGELOG.md`, tag
  `v1.0.0`.

**Status (2026-10-24):** done; tagged `v1.0.0`.

- Native interop accessors for every object, with mid-stream native
  recording, tested on both backends (ADR 0071).
- Open item 16 settled; the hardware-gated items deferred past v1.
- `README.md` and `CHANGELOG.md` written; header comments reviewed; version
  1.0.0.
- Final runs:
  - Linux (RADV): debug and release suites; every sample on Wayland, X11,
    and XCB.
  - Windows (RTX 2070 SUPER): debug and release suites on D3D12 and Vulkan;
    every sample on both backends.
  - Hosted CI: green.
- Names proposed during M8 were reviewed and settled (`docs/naming.md`).

## After v1.0

Mesh shaders and ray tracing (ADR 0005),
sparse resources (0026), external memory and OpenXR (0027), native device
import (0018), transparent windows (0056), the Vulkan 1.3 fallback
evaluation (CLAUDE.md §3), ARM64 support (0070).
