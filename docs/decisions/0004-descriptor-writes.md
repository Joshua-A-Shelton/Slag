# 0004 — Descriptor writes and views

**Status:** Accepted (2026-10-22). Copying superseded by ADR 0031 (no copies in v1).

## Decision

- Descriptor heaps expose **opaque, index-addressed slots**. Users write a
  descriptor *description* (texture/buffer/sampler + view parameters) to a
  slot index; Slag writes the native descriptor. Copying slots is also
  supported.
- Raw descriptor memory and per-backend descriptor sizes are **not** exposed.
- **No SRV/UAV view objects.** A shader-visible "view" is just a descriptor
  in a heap slot.
- The only view object is a small **attachment view** for render targets and
  depth-stencil (name TBD).

## Mapping

| | D3D12 | Vulkan |
|---|---|---|
| Write slot | `Create*View` into the slot's CPU handle | descriptor write into heap memory at slot offset |
| Copy slots | `CopyDescriptorsSimple` | memory copy within/between heaps |
| Attachment view | RTV/DSV in internal CPU-only heaps | `VkImageView` |

## Why

D3D12 shader-visible heaps can only be filled through `Create*View` /
`CopyDescriptors`, so a raw-memory write path is not portable. Attachments
are the one place both APIs require a separate native object.
