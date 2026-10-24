# 0040 — Depth and stencil state

**Status:** Accepted (2026-10-23). Names are placeholders.

Baked into the pipeline (ADR 0037).

| Option | Slag |
|---|---|
| Depth test | bool |
| Depth write | bool |
| Depth compare | shared compare enum (ADR 0033): Never, Less, Equal, LessEqual, Greater, NotEqual, GreaterEqual, Always |
| Stencil test | bool |
| Stencil operations | separate front and back: fail, depth-fail, pass, compare |
| Stencil ops | Keep, Zero, Replace, IncrementSaturate, DecrementSaturate, Invert, IncrementWrap, DecrementWrap |
| Stencil read/write masks | one each, shared by both faces |
| Depth bounds test | bool, requires the depth-bounds capability; min/max set while recording |

Maps directly to `D3D12_DEPTH_STENCIL_DESC2` and Vulkan
`VkPipelineDepthStencilStateCreateInfo`.

## Notes

- Reversed-Z needs nothing special (0–1 depth range on both).
- The depth attachment format is part of pipeline creation (render target
  formats item).
- Binding a pipeline that writes depth/stencil while the depth attachment is
  read-only is a validation error.
