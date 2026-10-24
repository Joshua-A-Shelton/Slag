# 0023 — Resource creation and placement

**Status:** Accepted (2026-10-22). Names are placeholders; "memory block"
is a working name chosen to avoid confusion with descriptor heaps.

## Decision

- **Placement field** in every resource create struct (buffer and all seven
  texture types), one of:
  - **Automatic:** a memory kind; D3D12MA / VMA decide whether to
    sub-allocate or give a dedicated allocation. No dedicated-allocation hint.
  - **Placed:** a memory block + offset.
- **Memory blocks:** created with size, memory kind, debug name; destroyed
  explicitly. Destroying a block with live resources is a validation error.
  Always created 4 MB-aligned.
- **Any resource type in any block.** D3D12 resource heap tier 2 joins the
  baseline (ADR 0012). Vulkan: Slag picks a memory type valid for every
  resource type of that kind.
- **Alignment** follows D3D12 on both backends: 64 KB, 4 KB for small
  textures when the query allows, 4 MB for MSAA. This also satisfies Vulkan
  `bufferImageGranularity`.
- **Size/alignment query** per create struct type, taking the same struct,
  creating nothing. Results are backend-specific.
- **Aliasing:** placed resources may overlap. Textures are activated by a
  barrier from `UNDEFINED`; buffers by a barrier syncing with the last use
  of the overlapping resource. Contents are undefined after switching.

## Mapping

| | D3D12 | Vulkan |
|---|---|---|
| Automatic | D3D12MA (`CreateCommittedResource3` or sub-allocation) | VMA |
| Memory block | `CreateHeap` (4 MB alignment) | `vkAllocateMemory` |
| Query | `GetResourceAllocationInfo2` | `vkGetDevice{Buffer,Image}MemoryRequirements` |
| Placed | `CreatePlacedResource2` | create + `vkBind{Buffer,Image}Memory2` |
