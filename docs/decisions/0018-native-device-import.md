# 0018 — Native device import and creation hooks

**Status:** Accepted (2026-10-22)

## Decision

- **Not in v1:** wrapping an existing native device, and hooks that let user
  code perform `vkCreateInstance` / `vkCreateDevice` (needed for OpenXR on
  Vulkan, `XR_KHR_vulkan_enable2`).
- v1 offers only `native()` access through the interop headers. For OpenXR
  on D3D12 that covers device selection (pick the listed device with the
  runtime's LUID) and handing the runtime the native device and graphics
  queue — but rendering into runtime-created swapchain images also needs
  native resource wrapping, which is not in v1 (ADR 0027). So OpenXR is
  not usable in v1 on either backend.
- **Implementation constraint:** each backend creates its native instance
  and device in exactly one internal place, so creation hooks can be added
  later without restructuring.
- When added, hooks and wrapping live only in the native interop headers.

External memory interop (video, CUDA) is a resource-design topic.
