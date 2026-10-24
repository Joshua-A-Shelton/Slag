# 0051 — Queries

**Status:** Accepted (2026-10-23). `QueryPool` is final (docs/naming.md);
other names are placeholders.

## Decision

- **Query pools (`QueryPool`):** created with a type and a count; explicit destroy;
  opaque pointer (ADR 0052). D3D12 query heap / Vulkan query pool.
- **Types:**
  - **Timestamp** (graphics and compute queues; copy queue is an optional
    capability). D3D12 `EndQuery(TIMESTAMP)`; Vulkan
    `vkCmdWriteTimestamp2` at the all-commands stage.
  - **Occlusion**, exact sample count. Begin/end inside a render pass,
    graphics queue only. D3D12 `OCCLUSION`; Vulkan `OCCLUSION` + `PRECISE`.
- **Results** go only into a buffer, as 64-bit values (D3D12
  `ResolveQueryData`; Vulkan `vkCmdCopyQueryPoolResults`). CPU reads go
  through a readback buffer and a fence.
- **Reset:** Slag resets every query at set creation, and on Vulkan records
  a reset of the same range right after each resolve. Rule: **a query must
  be resolved before it is written again** (validated).
- **Timestamp frequency:** per queue, ticks per second (D3D12
  `GetTimestampFrequency`; Vulkan from `timestampPeriod`).
- **Valid bits:** a **per-device value**, as a named exception to ADR 0028.
  The Vulkan spec allows 36–64, and pre-Xe2 Intel hardware has 36-bit
  timestamps (verified 2026-10-23). D3D12 reports 64.
- **CPU/GPU clock calibration:** optional capability (D3D12
  `GetClockCalibration`; Vulkan `VK_KHR_calibrated_timestamps`).

## Cut

Binary occlusion (results differ between backends), pipeline statistics
(result layouts differ), CPU-side `vkGetQueryPoolResults`.
