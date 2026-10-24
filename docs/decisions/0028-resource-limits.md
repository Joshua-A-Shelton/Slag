# 0028 — Resource limits

**Status:** Accepted (2026-10-22). Names are placeholders.

## Decision

- Limits are **fixed constants** in the public headers, using D3D12 feature
  level 12_0 values. No per-device limits query.
- Every constant is part of the baseline check (ADR 0012): a Vulkan device
  that reports a lower limit is not listed.
- Validation checks against the constants.

| Limit | Value |
|---|---|
| 1D texture width | 16384 |
| 2D texture width/height | 16384 |
| 3D texture size | 2048 |
| Cube face size | 16384 |
| Array layers | 2048 |
| Mip levels | 15 |
| MSAA samples | 8 |
| Constant buffer size | 64 KB (Vulkan `maxUniformBufferRange` ≥ 65536 required) |
| Constant buffer alignment | 256 bytes |
| Push constants | 128 bytes |

## Open

- **Maximum buffer size (decided 2026-10-23): a per-device value**, as a
  named exception to fixed limits, reported in the device's capabilities.
  D3D12 derives it from `MaxGPUVirtualAddressBitsPerResource` /
  `...PerProcess` (no fixed documented limit); Vulkan reads `maxBufferSize`.
  Creation can still fail with `OutOfMemory`, reported through `Result`.
  The reach of a single buffer descriptor is a separate limit (buffer view
  limits, below).
- Descriptor heap sizes: ADR 0029.

## Buffer view limits (accepted, 2026-10-23)

How much of a buffer one descriptor can cover. Fixed constants with Vulkan
baseline checks, like the other limits. Names are final (`docs/naming.md`).

| Descriptor kind | Proposed limit | D3D12 | Vulkan (spec minimum → desktop) |
|---|---|---|---|
| Constant buffer | 64 KB (`limits::maxConstantBufferSize`) | 4096 × 16 bytes | `maxUniformBufferRange` ≥ 65536 in 1.4 |
| Typed / RW typed | 2^27 elements (`limits::maxTypedBufferElements`) | 2^27 texels | `maxTexelBufferElements` ≥ 65536 → 2^27 or more; baseline check |
| Structured / RW structured | 2^27 elements **and** ≤ 1 GiB (`limits::maxStructuredBufferElements`, `limits::maxStorageBufferViewSize`) | 2^27 elements | `maxStorageBufferRange` (bytes) ≥ 2^27 → 1 GiB–4 GiB; baseline check |
| Raw / RW raw | 1 GiB (`limits::maxStorageBufferViewSize`) | 2^31 bytes guaranteed | same as above |

- 1 GiB is the conservative choice: some Vulkan drivers report 1 GiB for
  `maxStorageBufferRange`, while others report 4 GiB − 1. Larger data needs
  several descriptors (or GPU addresses, where that capability exists).
- To verify: whether `VK_EXT_descriptor_heap` address-range descriptors use
  the same limits, and Intel's reported values.
