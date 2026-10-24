# 0019 — Memory kinds

**Status:** Accepted (2026-10-22). Names are placeholders.

## Decision

| Kind | D3D12 | Vulkan |
|---|---|---|
| GPU only | `DEFAULT` | `DEVICE_LOCAL` |
| Upload (CPU writes, GPU reads) | `UPLOAD` | `HOST_VISIBLE \| HOST_COHERENT` |
| Readback (GPU writes, CPU reads) | `READBACK` | `HOST_VISIBLE \| HOST_COHERENT \| HOST_CACHED`, falling back to uncached |
| GPU upload (optional capability) | `GPU_UPLOAD` (`D3D12_OPTIONS16::GPUUploadHeapSupported`) | `DEVICE_LOCAL \| HOST_VISIBLE`, only when that heap is roughly VRAM-sized (full resizable BAR) |

- **Coherent memory only.** No flush/invalidate API.
- **Unified memory:** same four kinds; a `unifiedMemory` capability bool
  (D3D12 `ARCHITECTURE1::UMA`/`CacheCoherentUMA`, Vulkan heap layout) lets
  users skip staging copies.
- **Memory budget query:** budget and usage for VRAM and for system memory
  (DXGI `QueryVideoMemoryInfo`, `VK_EXT_memory_budget`).

## Cut

- D3D12 `CUSTOM` heaps; arbitrary Vulkan type combinations.
- The 256 MB non-resizable-BAR window (no D3D12 equivalent).
- `LAZILY_ALLOCATED`, `PROTECTED`.
- Residency (`MakeResident`/`Evict`) and memory priorities.
