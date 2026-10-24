# 0033 — Samplers

**Status:** Accepted (2026-10-23). Names are placeholders.

Written into the sampler heap with a per-kind write function (ADR 0031).

## Options

| Option | Slag | D3D12 | Vulkan |
|---|---|---|---|
| Filtering | separate min, mag, mip: Point / Linear | combined filter enum | separate fields |
| Anisotropy | `maxAnisotropy` 1 (off)–16; > 1 requires all Linear | anisotropic filter | `samplerAnisotropy` |
| Address | Wrap, Mirror, Clamp, Border, MirrorOnce | same five | `MIRROR_CLAMP_TO_EDGE` needs `samplerMirrorClampToEdge` (baseline check) |
| Border color | TransparentBlack, OpaqueBlack, OpaqueWhite (float) | float4 | fixed float colors |
| Comparison | optional compare function | comparison filter + func | `compareEnable` + `compareOp` |
| Min/max reduction | optional capability | min/max filter variants | `samplerFilterMinmax` |
| LOD | bias (constant range ±14, guaranteed by Vulkan 1.4; decided 2026-10-23), min LOD, max LOD | same (Slag validates ±14 on D3D12 too) | same; `maxSamplerLodBias` ≥ 14, no baseline check |

## Cut

Anisotropic with point mips; custom and integer border colors;
unnormalized coordinates; YCbCr conversion; **static samplers** (resolves
that half of the CLAUDE.md §14 question).

## Limit

D3D12 allows at most 4,096 unique sampler descriptions per device
(duplicates count once). Validation counts them.
