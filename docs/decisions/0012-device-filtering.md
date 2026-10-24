# 0012 — Filtering unsupported graphics devices

**Status:** Accepted (2026-10-22)

## Decision

- The instance lists only devices that meet the baseline. Each rejected
  device is logged through the debug callback (source Slag, severity Info)
  with the reason. No "supported" flag in the device info.
- The list is built once, at instance creation, and never changes.
  No hot-plug handling; recreate the instance to pick up changes.
- Zero supported devices is not an error: instance creation succeeds with
  an empty list.

## Baseline checks

| | D3D12 | Vulkan |
|---|---|---|
| API | feature level 12_0 | `apiVersion` ≥ 1.4 |
| Shaders | Shader Model 6.8 (ADR 0047) | — |
| Binding | resource binding tier 3 | `VK_EXT_descriptor_heap` + feature bit |
| Memory | resource heap tier 2 (ADR 0023) | — |
| Formats | `D3D12_OPTIONS12::RelaxedFormatCastingSupported` (ADR 0022) | `shaderStorageImageExtendedFormats`, `textureCompressionBC`, and one check per ◐ entry in `docs/format-support.md` |
| Barriers | `D3D12_OPTIONS12::EnhancedBarriersSupported` | (synchronization2 is core) |
| Queues | — | a queue family with graphics support |
| Software devices | excluded unless enabled | excluded unless enabled |

Every limit constant from ADR 0028 is also a baseline check on Vulkan,
including `maxTexelBufferElements` ≥ 2^27 and `maxStorageBufferRange` ≥ 1 GiB.

Presentation support is **not** part of the baseline; it is checked at
swapchain creation.

## D3D12 check cost

Checks need an `ID3D12Device`: Slag creates one per adapter, checks, and
releases it, so listed devices stay cheap. Initializing a device creates it
again (roughly tens of milliseconds).

## Added in M2 (2026-10-24)

- Vulkan: `separateDepthStencilLayouts` (core 1.2 feature), for per-plane
  depth/stencil barriers (ADR 0049).
- Both backends: every guaranteed (● and ◐) entry of
  `docs/format-support.md`, checked per format when devices are listed.
