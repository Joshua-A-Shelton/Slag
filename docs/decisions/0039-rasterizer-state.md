# 0039 — Rasterizer state

**Status:** Accepted (2026-10-23). Names are placeholders.

Baked into the pipeline (ADR 0037).

| Option | Slag | D3D12 | Vulkan |
|---|---|---|---|
| Fill mode | Solid, Wireframe | `FillMode` | `polygonMode` (`fillModeNonSolid` in baseline) |
| Cull mode | None, Front, Back | `CullMode` | `cullMode` |
| Front face | counter-clockwise flag | `FrontCounterClockwise` | `frontFace` |
| Depth bias | constant, clamp, slope scale | same | same (`depthBiasClamp` in baseline) |
| Depth clip | `depthClip` flag (on = clip, off = clamp) | `DepthClipEnable` | `depthClampEnable = !depthClip` (`depthClamp` in baseline) |
| Conservative rasterization | overestimate, optional capability | tiers | `VK_EXT_conservative_rasterization` |

## Cut

Point fill mode, FrontAndBack culling, line rasterization modes, forced
sample count, rasterizer discard, sample shading, custom sample locations,
conservative underestimation and tier differences.

## Known difference

Lines use each backend's default rasterization (D3D12 switches to
quadrilateral lines with MSAA; Vulkan depends on `strictLines`), so line
edges may differ slightly. Documented, not designed around.
