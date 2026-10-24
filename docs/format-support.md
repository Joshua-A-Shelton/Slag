# Guaranteed format support (draft for review)

**Status:** Accepted (2026-10-23). ◐ entries verified on every driver Slag
has run on (2026-10-24, M8; see "To verify"). ADR 0022.

What every graphics device that passes Slag's baseline check supports, per
`PixelFormat`. Anything not guaranteed here is queried per device with
`device->formatSupport(format)`.

## How this table was built

- **D3D12:** formats required at feature level 12.0 ("Hardware support for
  Direct3D 12.0 formats").
- **Vulkan:** the spec's mandatory format support tables, with the baseline
  features `shaderStorageImageExtendedFormats` and `textureCompressionBC`
  enabled.
- **●** Guaranteed: required by both.
- **◐** Guaranteed through a Slag baseline check: D3D12 requires it, Vulkan
  doesn't mandate it, and desktop drivers are expected to support it. Each ◐
  becomes a device-filtering check (ADR 0012). **To verify against real
  driver reports before treating as final.**
- **○** Query: optional on at least one backend; use `formatSupport()`.
- **—** Not supported (not allowed, or cut).

## Columns

| Column | Meaning | D3D12 | Vulkan |
|---|---|---|---|
| Smp | Sampled / loaded in shaders | sample, load | `SAMPLED_IMAGE` |
| Flt | Linear filtering | sample (filter) | `SAMPLED_IMAGE_FILTER_LINEAR` |
| RT | Render target | render target | `COLOR_ATTACHMENT` |
| Bld | Blending | blendable RT | `COLOR_ATTACHMENT_BLEND` |
| MS4 | 4× MSAA render/depth target | multisample RT 4× | `framebuffer*SampleCounts` |
| Wr | Shader write (`RWTexture*`) | typed UAV store | `STORAGE_IMAGE` |
| Ld | Shader read of a writable texture | typed UAV load | storage read without format |
| Atm | Atomics (`RWTexture*` / `RWBuffer`) | UAV atomics | `STORAGE_IMAGE_ATOMIC` / `STORAGE_TEXEL_BUFFER_ATOMIC` |
| DS | Depth/stencil target | depth/stencil | `DEPTH_STENCIL_ATTACHMENT` |
| VB | Vertex attribute | IA vertex buffer | `VERTEX_BUFFER` |
| TB | Typed buffer (`Buffer<T>`) | typed buffer load | `UNIFORM_TEXEL_BUFFER` |
| RWTB | Writable typed buffer (`RWBuffer<T>`) | typed UAV store | `STORAGE_TEXEL_BUFFER` |

Not in the table: 2× and 8× MSAA are always **○** (Vulkan only guarantees 1×
and 4×). Copies are supported for every format that is sampled.

## 8-bit

| Format | Smp | Flt | RT | Bld | MS4 | Wr | Ld | Atm | DS | VB | TB | RWTB |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| `R8Unorm` | ● | ● | ● | ● | ● | ● | ○ | — | — | — | ● | ○ |
| `R8Snorm` | ● | ● | ○ | ○ | ○ | ● | ○ | — | — | — | ● | ○ |
| `R8Uint`, `R8Sint` | ● | — | ● | — | ◐ | ● | ○ | — | — | — | ● | ○ |
| `R8G8Unorm` | ● | ● | ● | ● | ● | ● | ○ | — | — | ● | ● | ○ |
| `R8G8Snorm` | ● | ● | ○ | ○ | ○ | ● | ○ | — | — | ● | ● | ○ |
| `R8G8Uint`, `R8G8Sint` | ● | — | ● | — | ◐ | ● | ○ | — | — | ● | ● | ○ |
| `R8G8B8A8Unorm` | ● | ● | ● | ● | ● | ● | ○ | — | — | ● | ● | ● |
| `R8G8B8A8UnormSrgb` | ● | ● | ● | ● | ● | — | — | — | — | — | — | — |
| `R8G8B8A8Snorm` | ● | ● | ○ | ○ | ○ | ● | ○ | — | — | ● | ● | ● |
| `R8G8B8A8Uint`, `R8G8B8A8Sint` | ● | — | ● | — | ◐ | ● | ○ | — | — | ● | ● | ● |
| `B8G8R8A8Unorm` | ● | ● | ● | ● | ● | ○ | ○ | — | — | ● | ● | ○ |
| `B8G8R8A8UnormSrgb` | ● | ● | ● | ● | ● | — | — | — | — | — | — | — |

## 16-bit

| Format | Smp | Flt | RT | Bld | MS4 | Wr | Ld | Atm | DS | VB | TB | RWTB |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| `R16Unorm` | ◐ | ◐ | ◐ | ◐ | ◐ | ● | ○ | — | — | — | ○ | ○ |
| `R16Snorm` | ◐ | ◐ | ○ | ○ | ○ | ● | ○ | — | — | — | ○ | ○ |
| `R16Uint`, `R16Sint` | ● | — | ● | — | ◐ | ● | ○ | — | — | — | ● | ○ |
| `R16Float` | ● | ● | ● | ● | ● | ● | ○ | — | — | — | ● | ○ |
| `R16G16Unorm` | ◐ | ◐ | ◐ | ◐ | ◐ | ● | ○ | — | — | ● | ○ | ○ |
| `R16G16Snorm` | ◐ | ◐ | ○ | ○ | ○ | ● | ○ | — | — | ● | ○ | ○ |
| `R16G16Uint`, `R16G16Sint` | ● | — | ● | — | ◐ | ● | ○ | — | — | ● | ● | ○ |
| `R16G16Float` | ● | ● | ● | ● | ● | ● | ○ | — | — | ● | ● | ○ |
| `R16G16B16A16Unorm` | ◐ | ◐ | ◐ | ◐ | ◐ | ● | ○ | — | — | ● | ○ | ○ |
| `R16G16B16A16Snorm` | ◐ | ◐ | ○ | ○ | ○ | ● | ○ | — | — | ● | ○ | ○ |
| `R16G16B16A16Uint`, `R16G16B16A16Sint` | ● | — | ● | — | ◐ | ● | ○ | — | — | ● | ● | ● |
| `R16G16B16A16Float` | ● | ● | ● | ● | ● | ● | ○ | — | — | ● | ● | ● |

## 32-bit

| Format | Smp | Flt | RT | Bld | MS4 | Wr | Ld | Atm | DS | VB | TB | RWTB |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| `R32Uint`, `R32Sint` | ● | — | ● | — | ◐ | ● | ● | ● | — | ● | ● | ● |
| `R32Float` | ● | ◐ | ● | ◐ | ● | ● | ● | — | — | ● | ● | ● |
| `R32G32Uint`, `R32G32Sint` | ● | — | ● | — | ◐ | ● | ○ | — | — | ● | ● | ● |
| `R32G32Float` | ● | ◐ | ● | ◐ | ● | ● | ○ | — | — | ● | ● | ● |
| `R32G32B32Float`, `Uint`, `Sint` (vertex only) | — | — | — | — | — | — | — | — | — | ● | — | — |
| `R32G32B32A32Uint`, `R32G32B32A32Sint` | ● | — | ● | — | ◐ | ● | ○ | — | — | ● | ● | ● |
| `R32G32B32A32Float` | ● | ◐ | ● | ◐ | ● | ● | ○ | — | — | ● | ● | ● |

`R32Float` atomics are cut: D3D12 only guarantees exchange.

## Packed

| Format | Smp | Flt | RT | Bld | MS4 | Wr | Ld | Atm | DS | VB | TB | RWTB |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| `R10G10B10A2Unorm` | ● | ● | ● | ● | ● | ● | ○ | — | — | ● | ● | ○ |
| `R10G10B10A2Uint` | ● | — | ● | — | ◐ | ● | ○ | — | — | ○ | ● | ○ |
| `R11G11B10Float` | ● | ● | ◐ | ◐ | ◐ | ● | ○ | — | — | — | ● | ○ |
| `R9G9B9E5SharedExp` | ● | ● | — | — | — | — | — | — | — | — | — | — |

## Depth and stencil

| Format | Smp | Flt | RT | Bld | MS4 | Wr | Ld | Atm | DS | VB | TB | RWTB |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| `D16Unorm` | ● | ○ | — | — | ● | — | — | — | ● | — | — | — |
| `D32Float` | ● | ○ | — | — | ● | — | — | — | ◐ | — | — | — |
| `D32FloatS8Uint` | ◐ | ○ | — | — | ● | — | — | — | ◐ | — | — | — |

- Sampling a depth format reads the depth plane. On D3D12 Slag writes the
  descriptor with the matching color format (`R16_UNORM`, `R32_FLOAT`,
  `R32_FLOAT_X8X24_TYPELESS`).
- `D32Float` / `D32FloatS8Uint` depth targets: Vulkan only requires one of
  the 24-bit or 32-bit variants; desktop drivers expose the 32-bit ones
  (AMD exposes no 24-bit depth/stencil at all).

## Block-compressed

| Format | Smp | Flt | everything else |
|---|---|---|---|
| `BC1Unorm`, `BC1UnormSrgb`, `BC3Unorm`, `BC3UnormSrgb`, `BC4Unorm`, `BC4Snorm`, `BC5Unorm`, `BC5Snorm`, `BC6HUfloat16`, `BC6HSfloat16`, `BC7Unorm`, `BC7UnormSrgb` | ● | ● | — |

## Changes this implies elsewhere

- **Baseline (ADR 0012):** add Vulkan `shaderStorageImageExtendedFormats`
  and `textureCompressionBC`, plus one check per ◐ entry.
- **Vertex formats (ADR 0038):** `R10G10B10A2Uint` as a vertex attribute is
  not guaranteed (Vulkan doesn't require it), so it moves from "allowed" to
  "query".
- **Typed loads of writable textures:** only `R32Uint`, `R32Sint`,
  `R32Float` are guaranteed; D3D12 requires more, but Vulkan decides per
  format.

## To verify

Slag checks every ● and ◐ entry when it lists devices and skips a device
that misses one (ADR 0012), so a listed device is a verified one. Verified
(2026-10-24, M8) on RADV (RX 7900 XT, Raphael iGPU), NVIDIA RTX 2070 SUPER
(Vulkan and D3D12, driver 621.01), and WARP. That covers the atomics column
(`STORAGE_TEXEL_BUFFER_ATOMIC` on `R32Uint`/`R32Sint`), whose spec reading
was ambiguous.

Still open (deferred past v1, `docs/open-items.md`):

- Intel drivers, Windows and Linux.
- Depth comparison sampling on Vulkan depth formats: no format check or test
  yet.
