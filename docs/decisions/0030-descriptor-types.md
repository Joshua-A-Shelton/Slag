# 0030 — Resource descriptor types

**Status:** Accepted (2026-10-23). Names are placeholders.

Samplers: separate decision.

## Kinds

| Kind | HLSL | D3D12 | Vulkan | Parameters |
|---|---|---|---|---|
| Constant buffer | `ConstantBuffer<T>` | CBV | uniform buffer | address, size (multiple of 256, ≤ 64 KB) |
| Raw buffer | `ByteAddressBuffer` | raw SRV | storage buffer | buffer + offset, size |
| Structured buffer | `StructuredBuffer<T>` | structured SRV | storage buffer | buffer + offset, count, stride |
| Typed buffer | `Buffer<T>` | typed SRV | uniform texel buffer | buffer + offset, count, format |
| Writable raw | `RWByteAddressBuffer` | raw UAV | storage buffer | as raw |
| Writable structured | `RWStructuredBuffer<T>` | structured UAV | storage buffer | as structured |
| Writable typed | `RWBuffer<T>` | typed UAV | storage texel buffer | as typed |
| Texture | `Texture2D`, `TextureCube`, … | SRV | sampled image | view type, format, mip range, layer range, plane |
| Writable texture | `RWTexture2D`, … | UAV | storage image | view type, format, one mip, layer range / depth-slice range |

- Readable view types: 1D, 1D array, 2D, 2D array, 2D MSAA, 2D MSAA array,
  3D, cube, cube array. Writable: same minus MSAA.
- On Vulkan, `VK_EXT_descriptor_heap` takes the view parameters directly;
  no image/buffer view objects are created.

## Validation

- View type must match the texture type, except cube / cube array may also
  be viewed as 2D array.
- Format must be the texture's format or one of its castable formats.
- Typed buffer formats are limited to those the support table allows.

## Cut

UAV counters, component swizzle, min-LOD clamp on views, acceleration
structure descriptors (RT design).

## Raw buffer offset alignment (verified 2026-10-23)

Fixed constant: **256 bytes**. The Vulkan spec caps
`minStorageBufferOffsetAlignment` at 256, so every device satisfies it and
no baseline check is needed. D3D12's 16-byte rule is looser, so it's also
satisfied.
