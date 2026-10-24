# 0027 — External memory

**Status:** Accepted (2026-10-22)

## Decision

No external memory in v1:

- no wrapping of native buffers/textures created outside Slag
  (OpenXR swapchain images, video decoder output, host-engine resources);
- no cross-process / cross-API sharing through OS handles
  (D3D12 shared handles, Vulkan external memory / dma-buf, external
  semaphores);
- no import of CPU memory (`VK_EXT_external_memory_host`, Vulkan only).

## Consequence

OpenXR is not usable with Slag in v1 on either backend: rendering into
runtime-created swapchain images requires wrapping native resources.
(Corrects ADR 0018, which said D3D12 OpenXR already worked.)

## If added later

1. Wrapping native resources through the interop headers, using the
   matching create struct + native object; Slag doesn't own the memory;
   externally created Vulkan resources with `EXCLUSIVE` sharing usable only
   on the graphics queue.
2. OS-handle sharing: an "exportable" option in create structs plus
   export/import functions with a platform handle type.
