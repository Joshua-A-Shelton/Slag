# 0051 — Queries

**Status:** Accepted (2026-10-23). Names are final (docs/naming.md;
settled after M5).

## Decision

- **Query pools (`QueryPool`):** created with a type and a count; explicit destroy;
  opaque pointer (ADR 0052). D3D12 query heap / Vulkan query pool.
- **Types:**
  - **Timestamp** (graphics and compute queues; copy queue is an optional
    capability, `copyQueueTimestamps`). D3D12 `EndQuery(TIMESTAMP)`; Vulkan
    `vkCmdWriteTimestamp2` at the all-commands stage. A pool names the queue
    type it is used on (`QueryPoolDesc::queueType`): D3D12 keeps copy-queue
    timestamps in their own heap type (`COPY_QUEUE_TIMESTAMP`), so copy pools
    are only used on copy queues, and graphics/compute pools on either of
    those. Vulkan's `vkCmdCopyQueryPoolResults` and `vkCmdResetQueryPool`
    need a graphics or compute family, so a dedicated transfer family
    (NVIDIA's) reports no copy-queue timestamps (found 2026-10-24).
    Vulkan devices also need `timestampComputeAndGraphics` and
    `hostQueryReset` (baseline).
  - **Occlusion**, exact sample count. Begin/end inside a render pass,
    graphics queue only. D3D12 `OCCLUSION`; Vulkan `OCCLUSION` + `PRECISE`.
- **Results** go only into a buffer, as 64-bit values (D3D12
  `ResolveQueryData`; Vulkan `vkCmdCopyQueryPoolResults`). CPU reads go
  through a readback buffer and a fence.
- **Reset:** Slag resets every query at pool creation (Vulkan
  `vkResetQueryPool` on the host), and on Vulkan records a reset of the same
  range right after each resolve. Rule: **a query must be resolved before it
  is written again.** Slag doesn't validate it (record order isn't submit
  order); the Vulkan validation layers report writes to unreset queries.
- **Timestamp frequency:** per queue, ticks per second (D3D12
  `GetTimestampFrequency`; Vulkan from `timestampPeriod`).
- **Valid bits:** a **per-device value**, as a named exception to ADR 0028.
  The Vulkan spec allows 36–64, and pre-Xe2 Intel hardware has 36-bit
  timestamps (verified 2026-10-23). D3D12 reports 64.
- **CPU/GPU clock calibration:** optional capability `timestampCalibration`
  (D3D12 `GetClockCalibration` per queue; Vulkan
  `VK_KHR_calibrated_timestamps`, device and host domains, the same for every
  queue). `CommandQueue::calibrateTimestamps` returns a GPU timestamp and
  the CPU time in nanoseconds on the platform's monotonic clock
  (`QueryPerformanceCounter` on Windows, `CLOCK_MONOTONIC` on Linux), so it
  compares with `std::chrono::steady_clock` on MSVC and libstdc++.

## Cut

Binary occlusion (results differ between backends), pipeline statistics
(result layouts differ), CPU-side `vkGetQueryPoolResults`.
