# 0029 — Descriptor heap ownership and sizes

**Status:** Accepted (2026-10-23). Names are placeholders.

## Decision

- Each initialized graphics device owns **exactly one resource descriptor
  heap and one sampler descriptor heap**. No heap objects in the API.
- Users write descriptors by index on the device.
- Slag binds both heaps automatically at the start of every command list.
- Heap sizes are set at device initialization (alongside the optional
  pipeline cache blob, ADR 0043, and queue counts, ADR 0054; the only initialization
  options), with defaults:

| Heap | Default | Maximum (constant + baseline check) |
|---|---|---|
| Resource | 262,144 (2^18) | 1,000,000 |
| Sampler | 2048 | 2048 (D3D12 hard limit) |

Raising the resource heap to the maximum, expected to be the most common
choice, is one line using a public constant (names are placeholders):

```cpp
initDesc.resourceHeapSize = slag::kMaxResourceHeapSize;   // 1,000,000
```

Why the default is lower than the maximum: on Vulkan every slot is
max(`imageDescriptorSize`, `bufferDescriptorSize`), up to 128 bytes, so
1,000,000 slots can cost up to 128 MB in CPU-writable GPU memory, half of the
256 MB window on systems without resizable BAR. The default costs ~8 MB on
D3D12 and at most 32 MB on Vulkan.

## Not a hard requirement

The project owner prefers this design, but "no heap objects" is not a fixed
rule. If another part of the API turns out to be better served by exposing
heap objects (and binding them), revisit this decision.

## Mapping

| | D3D12 | Vulkan |
|---|---|---|
| Resource heap | one shader-visible CBV/SRV/UAV heap | resource heap (`VK_EXT_descriptor_heap`) |
| Sampler heap | one shader-visible sampler heap | sampler heap |
| Binding | `SetDescriptorHeaps` once per command list | heap bind once per command buffer |

## Why

Bindless engines already use one large heap of each kind, and heap
switching can stall the GPU on some D3D12 hardware. This removes heap
objects, handles, and bind calls from the API.

## Vulkan limits (verified 2026-10-23)

- `maxResourceHeapSize` ≥ (2^20 − 2^15) descriptors + reserved range, i.e.
  at least 1,015,808 user descriptors: the 1,000,000 maximum fits.
- `maxSamplerHeapSize` ≥ 4000 samplers + reserved range: 2048 fits.
- The spec warns that binding a new heap may have a high synchronization
  cost, so one heap per device is the right design.
- Slot stride on Vulkan is the largest descriptor size; shaders index the
  same way via Slang's `-spirv-unified-descriptor-heap-stride` (ADR 0034).
- Descriptor sizes on Vulkan are up to 64 bytes (images, texel buffers) and
  128 bytes (buffers), versus ~32 on D3D12. **A 1,000,000-slot resource heap
  can cost up to 128 MB on Vulkan**; hence the lower default above
  (decided 2026-10-23).
