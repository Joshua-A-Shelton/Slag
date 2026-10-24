# 0035 — Attachment views

**Status:** Accepted (2026-10-23). Names are placeholders.

## Decision

- An **explicit attachment view object** (opaque pointer, ADR 0052), created and
  destroyed by the user. The only view object in Slag (ADR 0004).
- Create struct: texture, format (own or castable), one mip level, layer
  range (depth-slice range for 3D, face range for cubes), debug name.
- **No view-type field:** the native view type is derived from the
  texture's type and the layer count.
- **No read-only flags:** read-only depth/stencil is chosen when rendering
  begins (command-list design). On D3D12 Slag creates the up-to-four DSV
  variants per depth view internally.
- Destroying a texture with live views is a validation error. Users keep
  views alive until the GPU is done with them (fences).
- Swapchains provide attachment views for their images (swapchain design).

## Mapping

| | D3D12 | Vulkan |
|---|---|---|
| Color | RTV in an internal CPU-only RTV heap | `VkImageView` |
| Depth/stencil | DSV variants in an internal CPU-only DSV heap | `VkImageView` |
| Use | `OMSetRenderTargets` | `VkRenderingAttachmentInfo` |

## Not chosen

Passing texture/mip/layers directly when rendering begins with an internal
view cache (hidden lookups and allocations), and automatic default views.
