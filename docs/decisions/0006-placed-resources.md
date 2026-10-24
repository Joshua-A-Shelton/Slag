# 0006 — Placed resources and aliasing

**Status:** Accepted (2026-10-22)

## Decision

v1 exposes explicit memory heaps, placed resources, and aliasing, alongside a
convenience "committed" creation path.

## Mapping

| | D3D12 | Vulkan |
|---|---|---|
| Heap | `ID3D12Heap` (via D3D12MA) | `VkDeviceMemory` (via VMA) |
| Placed resource | `CreatePlacedResource` | create + bind buffer/image memory at offset |
| Alias activation | Enhanced Barriers layout from `UNDEFINED` with discard | barrier from `VK_IMAGE_LAYOUT_UNDEFINED` |

## Why

Explicit memory is a core goal, the mapping is direct, and both allocator
libraries already support it.
