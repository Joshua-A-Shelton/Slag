# 0058 — Resizing and reconfiguring swapchains

**Status:** Accepted (2026-10-23). Names are placeholders. Resolves the
image-count question in ADR 0056.

## Detecting a size mismatch

- D3D12: at each present Slag compares the swapchain size with the window's
  client area (`GetClientRect`) and returns `Suboptimal` on mismatch.
- Vulkan: `SUBOPTIMAL` / `OUT_OF_DATE` from acquire or present.
- Handling is the same on both: on `Suboptimal` or `OutOfDate`, call
  `reconfigure` with the new size.
- Documented differences: after `OutOfDate` on Vulkan the frame wasn't
  shown (D3D12 shows it scaled); on Wayland no mismatch is detected and the
  user drives size from compositor configure events.

## `swapchain->reconfigure(desc)`

- Changes width, height, image count, format, and present mode (needed for
  modes outside the switchable set, ADR 0056); the castable sRGB format
  carries over. This is how image count changes after
  any present.
- D3D12 `ResizeBuffers`; Vulkan recreate with `oldSwapchain`, reusing the
  surface.
- Preconditions (validated): the GPU is done with every swapchain image
  (user waits on fences or `queue->waitIdle()`; Slag doesn't wait); no image
  is acquired but unpresented.
- Earlier image `Texture*` / attachment view pointers become invalid.

## Minimized windows

- `reconfigure` with a zero size is an error.
- Present returns a new `Occluded` result when the window isn't visible
  (D3D12 `DXGI_STATUS_OCCLUDED`; Vulkan: surface reports zero size).
  Users skip rendering and reconfigure once the window has a size.

## Cut

Automatic resizing / watching window events.
