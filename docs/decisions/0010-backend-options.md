# 0010 — Backend-specific options

**Status:** Accepted (2026-10-22). Names are placeholders.

## Decision

- The instance description contains the backend tag, common options, and a
  **union of per-backend option structs**. The tag selects the active member.
- Option structs use **plain types only** (no native handles or SDK types),
  since they appear in public headers.
- Console option structs are added to the union through a build-time
  include from the private backend directory. The description's size may
  therefore differ between console and public builds (no stable ABI across
  build configurations).
- In v1 the D3D12 and Vulkan members are **empty**. Everything needed turned
  out to be common or cuttable.

## Common options

| Option | D3D12 | Vulkan |
|---|---|---|
| App/engine name and version | ignored | `VkApplicationInfo` |
| Validation flags: API, GPU-assisted, synchronization | debug layer, GPU-based validation; synchronization unsupported → reported | validation layer, GPU-AV, sync validation |
| Software devices allowed | WARP | lavapipe / SwiftShader |
| Crash diagnostics (ADR 0014) | DRED | `VK_EXT_device_fault` |

Validation flags are **best effort**: a flag a backend can't honor is
reported through the debug callback, never silently dropped.

## Cut

- Agility SDK path/version: the application exports `D3D12SDKVersion` /
  `D3D12SDKPath` using a macro Slag provides.
- Extra Vulkan instance/device extensions: the real need (OpenXR, CUDA
  interop) is native import, handled separately.
- Validation sub-modes, break-on-severity, stable power state, experimental
  features, loader/ICD selection.
