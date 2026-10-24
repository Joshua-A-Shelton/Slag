# 0006 — Placed resources and aliasing

**Status:** Accepted (2026-10-22). Allocator libraries replaced by Slag's own allocator (ADR 0061).

## Decision

v1 exposes explicit memory heaps, placed resources, and aliasing, alongside a
convenience "committed" creation path.

## Mapping

| | D3D12 | Vulkan |
|---|---|---|
| Heap | `ID3D12Heap` | `VkDeviceMemory` |
| Placed resource | `CreatePlacedResource` | create + bind buffer/image memory at offset |
| Alias activation | Enhanced Barriers layout from `UNDEFINED` with discard | barrier from `VK_IMAGE_LAYOUT_UNDEFINED` |

## Why

Explicit memory is a core goal, the mapping is direct, and both allocator
libraries already support it.
