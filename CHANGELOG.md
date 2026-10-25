# Changelog

Slag follows semantic versioning (ADR 0070). There is no stable ABI.

## 1.0.0 — unreleased

The first release: a thin C++20 abstraction over Direct3D 12 (Agility SDK
1.619.6, Shader Model 6.8, Enhanced Barriers) and Vulkan 1.4 with
`VK_EXT_descriptor_heap`, on Windows and Linux.

### Core

- Instances choose a backend at creation and list the devices that meet the
  baseline; devices are initialized with a chosen number of graphics,
  compute, and copy queues (ADRs 0008, 0012, 0054).
- One function table per backend, called directly when one backend is
  compiled in; no virtual dispatch (ADRs 0001, 0065).
- `Result<T>` and `Status` everywhere, `[[nodiscard]]`; no exceptions, no
  RTTI (ADR 0060).
- The debug callback receives a `DebugMessage` struct, so fields can be
  added later (ADR 0011).
- CPU allocations through `AllocatorCallbacks` (ADR 0066).
- Capabilities for optional features, never silent emulation (ADR 0015).

### Resources and memory

- Buffers and textures (1D, 2D, 3D, cube, arrays, MSAA; textures report
  what they were created with), with automatic placement in Slag's TLSF sub-allocator, placement in user memory blocks,
  aliasing, and dedicated allocations for large resources (ADRs 0006, 0023,
  0061).
- Memory kinds GpuOnly, Upload, Readback, and GpuUpload; memory budgets
  (ADR 0019).
- Formats with a guaranteed support table and per-device queries (ADR 0022,
  `docs/format-support.md`).
- Defragmentation passes for movable resources, with stable pointers:
  `recordCopies` takes each moved texture's layout and records the barriers
  (ADR 0062).

### Descriptors and pipelines

- Bindless only: one resource heap and one sampler heap per device, written
  per descriptor kind at an index (ADRs 0029, 0031).
- A fixed pipeline layout: 128 bytes of push constants and a root constant
  buffer at `register(b1)` (ADRs 0003, 0044).
- Graphics and compute pipelines from DXIL and SPIR-V, compiled with Slang
  (ADR 0034); a pipeline cache blob (ADR 0043).

### Commands and synchronization

- Dynamic rendering, including render passes split across command buffers
  (ADR 0046); draws, indexed and indirect draws with GPU counts, dispatches
  (ADR 0047).
- Copies between buffers and textures, and MSAA resolves (ADR 0048).
- Enhanced-Barriers-style barriers on both backends (ADR 0049). A submit
  starts after earlier work on its queue has finished and is visible, as at
  D3D12's `ExecuteCommandLists` boundaries.
- Timeline semaphores for CPU and queue waits: `semaphore->wait()`, and
  `slag::waitSemaphores()` for all or any of several.
- Timestamp and occlusion queries, and CPU/GPU clock calibration
  (ADR 0051).
- Debug labels (ADR 0050).

### Presentation

- Swapchains on Win32, Xlib, XCB, and Wayland windows, with switchable present
  modes, resizing (`resize` keeps every other setting), HDR color spaces and metadata, tearing, and frame latency
  control (ADRs 0055–0059).

### Debugging

- Slag validation of every call in validation builds, with a stale-pointer
  quarantine and descriptor-slot tracking (D3D12 poison descriptors); all
  412 checks are tested without a GPU (ADRs 0016, 0031, 0052, 0067).
- Debug names on every object, in every build (ADR 0017).
- Crash diagnostics: D3D12 DRED and Vulkan `VK_EXT_device_fault` reports on
  device loss (ADR 0014).
- Native interop: every object's native handles, and native commands
  recorded mid-stream (ADR 0071).

### Render graph (optional module)

- `slag::rendergraph`: passes declare resource uses; the graph records
  barriers, splits submits with semaphores between queues, begins and ends
  rendering, places transient resources in shared memory, lets passes
  record on several threads, tracks textures per mip and layer, creates
  transient 2D, array, cube, cube-array, and 3D textures, and runs
  defragmentation passes after a frame (ADR 0072).

### Utilities and samples

- `util/DescriptorAllocator.h` and `util/DeferredDestroyer.h` (ADRs 0032,
  0069).
- Samples: triangle, textured_quad, compute, bindless, multi_queue,
  render_graph, crash.

### Not in 1.0

Native object import, mesh shaders, ray tracing, sparse resources, external
memory and OpenXR, transparent windows, render-pass resolves (cut, ADR
0049), and the Vulkan 1.3 fallback. The planned work after 1.0 is listed in
`docs/milestones.md` ("After v1.0").

### Tested on

RADV (RX 7900 XT, Raphael iGPU) on Linux; NVIDIA RTX 2070 SUPER (driver
621.01) on Windows 11 with D3D12 and Vulkan; WARP in hosted CI.
