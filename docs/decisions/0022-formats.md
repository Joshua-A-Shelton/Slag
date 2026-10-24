# 0022 — Formats

**Status:** Accepted (2026-10-22). Enum type name: `PixelFormat` (docs/naming.md).

Vertex formats: pipeline design. Swapchain formats: swapchain design.

## Format list (58)

Bit layouts follow DXGI; each maps to the matching Vulkan format.

| Group | Formats | Count |
|---|---|---|
| 8-bit | R8, RG8: UNORM/SNORM/UINT/SINT. RGBA8: same + UNORM_SRGB. BGRA8: UNORM, UNORM_SRGB | 15 |
| 16-bit | R16, RG16, RGBA16: UNORM/SNORM/UINT/SINT/FLOAT | 15 |
| 32-bit | R32, RG32, RGBA32: UINT/SINT/FLOAT | 9 |
| Packed | RGB10A2 UNORM/UINT; RG11B10 FLOAT; RGB9E5 shared exponent (sampling only) | 4 |
| Depth/stencil | D16_UNORM, D32_FLOAT, D32_FLOAT_S8_UINT | 3 |
| Block-compressed | BC1, BC3, BC7: UNORM/SRGB. BC4, BC5: UNORM/SNORM. BC6H: UF16/SF16 | 12 |

Vertex-only additions (ADR 0038): `R32G32B32Float`, `R32G32B32Uint`,
`R32G32B32Sint`.

Packed-format mapping examples: DXGI `R10G10B10A2_UNORM` → Vulkan
`A2B10G10R10_UNORM_PACK32`; `R11G11B10_FLOAT` → `B10G11R11_UFLOAT_PACK32`;
`R9G9B9E5_SHAREDEXP` → `E5B9G9R9_UFLOAT_PACK32`; `D32_FLOAT_S8X24_UINT` →
`D32_SFLOAT_S8_UINT`.

## Value naming (accepted)

Hybrid PascalCase: channel bits spelled out as in DXGI, type suffix in
PascalCase. Examples: `R8G8B8A8Unorm`, `R8G8B8A8UnormSrgb`, `B8G8R8A8Unorm`,
`R11G11B10Float`, `R9G9B9E5SharedExp`, `D32FloatS8Uint`, `BC4Snorm`,
`BC6HUfloat16`, `BC6HSfloat16`, `BC7UnormSrgb`.

## Cut

D24_UNORM_S8_UINT (no AMD Vulkan support), BC2, RGB32 as a texture format,
565/5551/4444, typeless formats, A8, R1, XR_BIAS, shared-channel packed
formats, S8, video/YUV formats, ASTC, ETC2.

## Format casting

- Optional list of castable formats in each texture create struct
  (D3D12 relaxed format casting via `CreateCommittedResource3` /
  `CreatePlacedResource2`; Vulkan `MUTABLE_FORMAT` +
  `VkImageFormatListCreateInfo`).
- Allowed: sRGB ↔ linear of the same format; uncompressed formats with the
  same bits per texel.
- Not in v1: block-compressed ↔ uncompressed views.
- Relaxed format casting is a separate optional D3D12 capability
  (`RelaxedFormatCastingSupported`), not implied by Enhanced Barriers, so it
  is a baseline check (ADR 0012). Verified 2026-10-23.

## Format support

- A **guaranteed support table** (render target, blend, shader write, typed
  load, filtering, MSAA counts, depth) valid on every baseline device. Built
  from D3D12 required support ∩ desktop Vulkan reports. Accepted:
  `docs/format-support.md` (◐ entries still to verify).
- A **per-format query** for support beyond the table.
