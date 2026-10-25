# 0021 — Textures

**Status:** Accepted (2026-10-22). Names are placeholders.

## Texture types

1D, 1D array, 2D, 2D array, 3D, cube, cube array — all explicit.

| Type | D3D12 | Vulkan |
|---|---|---|
| 1D / 1D array | `TEXTURE1D` | `VK_IMAGE_TYPE_1D` |
| 2D / 2D array | `TEXTURE2D` | `VK_IMAGE_TYPE_2D` |
| 3D | `TEXTURE3D` | `VK_IMAGE_TYPE_3D` (+ `2D_ARRAY_COMPATIBLE` when it's a render target) |
| Cube / cube array | `TEXTURE2D` with 6×N layers | `VK_IMAGE_TYPE_2D` + `CUBE_COMPATIBLE`, 6×N layers |

## Creation

**One create function per texture type**, each taking a struct with only the
fields relevant to that type. No single catch-all texture description.

Every type also takes: format, mip count, usage flags, debug name, memory
placement.

| Type | Size fields | Sample count | RenderTarget | DepthStencil | ShaderWrite | Clear value |
|---|---|---|---|---|---|---|
| 1D | width | — | ✓ | — | ✓ | ✓ |
| 1D array | width, layers | — | ✓ | — | ✓ | ✓ |
| 2D | width, height | ✓ | ✓ | ✓ | ✓ | ✓ |
| 2D array | width, height, layers | ✓ | ✓ | ✓ | ✓ | ✓ |
| 3D | width, height, depth | — | ✓ | — | ✓ | ✓ |
| Cube | size (square faces) | — | ✓ | ✓ | ✓ | ✓ |
| Cube array | size, cube count (layers = 6 × count) | — | ✓ | ✓ | ✓ | ✓ |

The clear value is optional and only accepted with RenderTarget or
DepthStencil.

## Object type

**One texture type** (`Texture*`, ADR 0052) for all texture types. Slag records each
texture's type; validation checks type-specific uses (e.g. a cube descriptor
from a non-cube texture).

## Usage flags

| Flag | D3D12 | Vulkan |
|---|---|---|
| RenderTarget | `ALLOW_RENDER_TARGET` | `COLOR_ATTACHMENT` |
| DepthStencil | `ALLOW_DEPTH_STENCIL` | `DEPTH_STENCIL_ATTACHMENT` |
| ShaderWrite | `ALLOW_UNORDERED_ACCESS` | `STORAGE` |
| *(always)* | — | `SAMPLED`, `TRANSFER_SRC`, `TRANSFER_DST` |

RenderTarget and DepthStencil are mutually exclusive.

## Other rules

- Mip count is explicit; no "0 = full chain".
- MSAA: 1, 2, 4, 8 samples; standard sample pattern
  (`D3D12_STANDARD_MULTISAMPLE_PATTERN`); only with RenderTarget or
  DepthStencil.
- Optimal layout only.
- Textures start in `UNDEFINED` layout with undefined contents (D3D12 uses
  `CREATE_NOT_ZEROED`).
- Optional optimized clear value: used by D3D12, ignored by Vulkan.

## Cut

`DENY_SHADER_RESOURCE`; transient/input attachments; 16× MSAA; quality
levels; MSAA with ShaderWrite; linear / row-major / standard-swizzle layouts;
simultaneous access; cross-adapter; sampler feedback; `PREINITIALIZED`;
remaining Vulkan creation flags (Slag sets `CUBE_COMPATIBLE` and
`2D_ARRAY_COMPATIBLE` internally where needed).

## Getters (added before v1.0, 2026-10-24)

`texture->format()`, `width()`, `height()`, `depth()`, `layerCount()`,
`mipCount()`, `sampleCount()`, `usage()`: what the texture was created with
(cubes count six layers per cube; 3D textures one layer and `depth`
slices). Read-only and thread-safe. Added for the render graph, which
tracks imported textures per mip and layer through the public API; the
texture type isn't exposed (it would need a new public enum).
