# 0003 — Fixed pipeline layout

**Status:** Accepted (2026-10-22). Amended by ADR 0044 (root constant buffer slot); static samplers cut by ADR 0033.

## Decision

Every pipeline uses one fixed, implicit layout:

1. the resource descriptor heap,
2. the sampler descriptor heap,
3. a fixed block of push constants — **128 bytes** initially.

No user-defined layouts and no pipeline-layout object in the API.

Deferred (can be added later without breaking the layout): a per-draw root
constant buffer, static/immutable samplers.

## Mapping

| | D3D12 | Vulkan 1.4 + `VK_EXT_descriptor_heap` |
|---|---|---|
| Resource heap | `ResourceDescriptorHeap` (SM 6.6, heap-directly-indexed root signature flag) | resource heap binding |
| Sampler heap | `SamplerDescriptorHeap` | sampler heap binding |
| Push constants | 32-bit root constants (one shared root signature) | push data / push constants |

## Why

D3D12 caps root signatures at 64 DWORDs (256 bytes) and Vulkan 1.4 guarantees
≥ 256 bytes of push constants. 128 bytes leaves D3D12 root-signature headroom
for later additions. One layout means pipelines need no layout object and all
shaders share one binding convention.

## Vulkan details (verified 2026-10-23)

- Heaps are bound with `vkCmdBindResourceHeapEXT` / `vkCmdBindSamplerHeapEXT`.
- Push constants become **push data** (`vkCmdPushDataEXT`), which is
  mutually exclusive with `vkCmdPushConstants` and push descriptors.
  `maxPushDataSize` ≥ 256, enough for 128 bytes of constants plus the root
  constant buffer address (ADR 0044).
- Pipelines are created with `VK_PIPELINE_CREATE_2_DESCRIPTOR_HEAP_BIT_EXT`
  and a `NULL` pipeline layout.
