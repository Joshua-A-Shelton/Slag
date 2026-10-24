# 0037 — Pipeline kinds and dynamic state

**Status:** Accepted (2026-10-23). Names are placeholders.

## Kinds (v1)

- **Graphics:** vertex shader + optional pixel shader (omitted for
  depth-only passes).
- **Compute.**
- Cut: geometry shaders, tessellation (hull/domain).
- Later, as optional capabilities: mesh shaders, then ray tracing (ADR 0005).

## Creation

- One create function per kind (graphics, compute).
- One pipeline type (`Pipeline*`, ADR 0052); Slag records the kind and
  validation checks use.

## Set while recording vs. baked

Rule: state D3D12 sets on the command list is set while recording;
everything else is baked into the pipeline.

| State | Slag |
|---|---|
| Viewports, scissors | while recording, up to 16 (Vulkan `multiViewport` in baseline) |
| Blend constant | while recording |
| Stencil reference | while recording, one value for both faces |
| Depth bounds | while recording, optional capability |
| Primitive topology | baked in full (e.g. TriangleList); D3D12 sets it at pipeline bind |
| Depth bias | baked |
| Stencil read/write masks | baked, one mask for both faces |
| Sample mask | baked |
| Everything else (cull, front face, depth/stencil ops, …) | baked |

Cut: switching list/strip while recording, primitive restart, line width.

## Implementation notes (M3)

- Viewports and scissors use a top-left origin, and clip-space +Y is up,
  on both backends: Vulkan viewports are flipped (negative height, core
  since 1.1), so the same shader and viewport give the same image.
- `beginRendering` sets one viewport and scissor covering the render area,
  since both APIs need them before drawing; set others after it.
- Vulkan uses `VIEWPORT_WITH_COUNT` / `SCISSOR_WITH_COUNT` (core 1.3), so
  pipelines don't fix the viewport count.

