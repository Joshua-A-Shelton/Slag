# 0041 — Blend state

**Status:** Accepted (2026-10-23). Names are placeholders.

Baked into the pipeline, except the blend constant (set while recording,
ADR 0037).

## Decision

- Up to **8 render targets** (constant + baseline check).
- **Always independent** blend per target (`independentBlend` in baseline).
- Per target: blend enable; source/destination factors for color and alpha;
  color and alpha ops; RGBA write mask.
- **Ops:** Add, Subtract, ReverseSubtract, Min, Max.
- **Factors:** Zero, One, SrcColor, InvSrcColor, SrcAlpha, InvSrcAlpha,
  SrcAlphaSaturate, DestColor, InvDestColor, DestAlpha, InvDestAlpha,
  BlendConstant, InvBlendConstant, Src1Color, InvSrc1Color, Src1Alpha,
  InvSrc1Alpha.
- **Alpha-to-coverage:** kept.
- **Dual-source blending:** kept (`dualSrcBlend` in baseline); render target
  0 only (validated).
- Blending on a format without blend support is a validation error
  (support table, ADR 0022).

## Cut

Logic ops (per-target in D3D12 vs. global in Vulkan), constant-alpha factor
(`ALPHA_FACTOR` / `CONSTANT_ALPHA`), Vulkan advanced blend ops.
