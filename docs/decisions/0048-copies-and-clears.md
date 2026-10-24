# 0048 — Copies and clears

**Status:** Accepted (2026-10-23). Names are placeholders.

## Copies

| Slag | D3D12 | Vulkan |
|---|---|---|
| `copyBuffer(dst, dstOffset, src, srcOffset, size)` | `CopyBufferRegion` | `vkCmdCopyBuffer2` |
| `copyTexture(dst, dstMip, dstLayer, dstXYZ, src, srcMip, srcLayer, srcXYZ, extent)` (same or castable formats) | `CopyTextureRegion` | `vkCmdCopyImage2` |
| `copyBufferToTexture` / `copyTextureToBuffer` | `CopyTextureRegion` with footprint | `vkCmdCopyBufferToImage2` / `vkCmdCopyImageToBuffer2` |
| `resolveTexture(dst, src)` (color average, graphics queue only) | `ResolveSubresource` | `vkCmdResolveImage2` |

- Buffer↔texture copies use D3D12 alignment on both backends: buffer offset
  multiple of 512, row pitch multiple of 256.
- Depth/stencil copies take a plane (depth or stencil).
- A device **footprint query** returns the buffer size and row pitch for one
  mip of a texture (`GetCopyableFootprints`).
- All three queue types can copy.

## Clears

- **No standalone render-target/depth clears.** Clear with a render pass
  load op (ADR 0046); D3D12 and Vulkan require different layouts for
  standalone clears.
- **No buffer fills** and no clears of shader-writable textures in v1.
  Users clear with a compute shader or a copy from a zeroed buffer.

## Cut

Scaled/filtered blits, MSAA copies, compressed ↔ uncompressed copies,
inline CPU-to-buffer updates, relaxed row-pitch capability, `fillBuffer`
(would need hidden descriptor slots on D3D12), writable-texture clears.

## Implementation (M2)

- Copies take `TextureLocation` and `BufferLocation` structs (names in
  review).
- `copyFootprint` is computed in common code, so the buffer layout is the
  same on both backends: rows of texels (or 4x4 blocks) padded to 256 bytes,
  depth slices `rowPitch * rowCount` apart. Depth/stencil textures copy one
  plane at a time: depth as 2 or 4 bytes, stencil as 1.
