# 0025 — GPU addresses

**Status:** Accepted (2026-10-22). Names are placeholders.

## Decision

- Every buffer exposes a 64-bit **GPU address** (D3D12
  `GetGPUVirtualAddress`, Vulkan `vkGetBufferDeviceAddress`).
- Addresses may be passed to shaders (e.g. in push constants) and
  dereferenced there. Today only Vulkan can do this, so it is gated by a
  **capability** (false on D3D12 until it supports it, expected with Shader
  Model 7 / SPIR-V). This is a deliberate exception to ADR 0015's
  "D3D12 feature set is the reference" rule.
- Ray tracing build inputs will use addresses (both APIs do natively).

## Why

D3D12 Shader Model 7 is moving to SPIR-V, and shader pointers on D3D12 are
expected to follow. Addresses are needed for acceleration structures anyway.

## Address vs. buffer + offset per call (accepted)

Converting buffer + offset → address is trivial (base + offset). The
reverse (address → native buffer + offset) needs a range lookup per call.

Rule: a call takes **buffer + offset** when any backend needs the native
buffer object, and an **address** only when every backend accepts one.

| Call | D3D12 needs | Vulkan needs | Slag takes |
|---|---|---|---|
| Copies | resource | buffer | buffer + offset |
| Indirect arguments/count | resource | buffer | buffer + offset |
| Vertex / index binding | address | buffer | buffer + offset |
| Buffer SRV/UAV descriptors | resource + element range | address range (`VkDeviceAddressRangeEXT`; texel buffers add a format) | buffer + offset |
| Constant buffer descriptors | address | address range | address |
| Push constants / shader data | — | — | address (capability-gated) |
| RT build inputs | address | address | address |
