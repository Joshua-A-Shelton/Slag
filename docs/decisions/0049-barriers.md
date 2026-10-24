# 0049 — Barriers

**Status:** Accepted (2026-10-23). Names are placeholders.

Modeled on D3D12 Enhanced Barriers; mapped to Vulkan synchronization2.

## Shape

```cpp
struct GlobalBarrier  { Sync syncBefore, syncAfter; Access accessBefore, accessAfter; };
struct BufferBarrier  { Sync syncBefore, syncAfter; Access accessBefore, accessAfter; Buffer* buffer; };
struct TextureBarrier { Sync syncBefore, syncAfter; Access accessBefore, accessAfter;
                        Layout layoutBefore, layoutAfter; Texture* texture; SubresourceRange range; };

cmd->barrier(std::span<const GlobalBarrier>, std::span<const BufferBarrier>, std::span<const TextureBarrier>);
```

- One call → `ID3D12GraphicsCommandList7::Barrier` / `vkCmdPipelineBarrier2`.
- Buffer barriers cover the whole buffer (D3D12 requirement).
- Subresource range: mips, layers, plane.
- A transition from `Undefined` always discards (Slag sets D3D12's discard
  flag). This is also how aliased resources are activated (ADR 0023).

## Sync

| Slag | D3D12 | Vulkan |
|---|---|---|
| None | `NONE` | `NONE` |
| All | `ALL` | `ALL_COMMANDS` |
| Draw | `DRAW` | `ALL_GRAPHICS` |
| IndexInput | `INDEX_INPUT` | `INDEX_INPUT` + `VERTEX_ATTRIBUTE_INPUT` |
| VertexShading | `VERTEX_SHADING` | `VERTEX_SHADER` |
| PixelShading | `PIXEL_SHADING` | `FRAGMENT_SHADER` |
| DepthStencil | `DEPTH_STENCIL` | `EARLY_` + `LATE_FRAGMENT_TESTS` |
| RenderTarget | `RENDER_TARGET` | `COLOR_ATTACHMENT_OUTPUT` |
| ComputeShading | `COMPUTE_SHADING` | `COMPUTE_SHADER` |
| AllShading, NonPixelShading | same | combined shader stages |
| Copy | `COPY` | `COPY` |
| Resolve | `RESOLVE` | `RESOLVE` (+ `COLOR_ATTACHMENT_OUTPUT` for in-pass resolves) |
| ExecuteIndirect | `EXECUTE_INDIRECT` | `DRAW_INDIRECT` |

## Access

NoAccess, VertexBuffer, IndexBuffer, ConstantBuffer, ShaderRead, ShaderWrite,
RenderTarget, DepthStencilRead, DepthStencilWrite, IndirectArgument,
CopySource, CopyDest, ResolveSource, ResolveDest. ShaderRead →
`SHADER_SAMPLED_READ | SHADER_STORAGE_READ | UNIFORM_READ`; ShaderWrite →
`SHADER_STORAGE_WRITE`.

## Layouts

| Slag | D3D12 | Vulkan |
|---|---|---|
| Undefined | `UNDEFINED` | `UNDEFINED` |
| Common | `COMMON` | `GENERAL` |
| Present | `PRESENT` | `PRESENT_SRC_KHR` |
| RenderTarget | `RENDER_TARGET` | `COLOR_ATTACHMENT_OPTIMAL` |
| DepthStencilWrite | `DEPTH_STENCIL_WRITE` | `DEPTH_STENCIL_ATTACHMENT_OPTIMAL` |
| DepthStencilRead | `DEPTH_STENCIL_READ` | `READ_ONLY_OPTIMAL` |
| ShaderRead | `SHADER_RESOURCE` | `READ_ONLY_OPTIMAL` |
| ShaderWrite | `UNORDERED_ACCESS` | `GENERAL` |
| CopySource / CopyDest | `COPY_SOURCE` / `COPY_DEST` | `TRANSFER_SRC/DST_OPTIMAL` |
| ResolveSource / ResolveDest | `RESOLVE_SOURCE` / `RESOLVE_DEST` | `TRANSFER_SRC/DST_OPTIMAL` |

## Queues

- `CONCURRENT` sharing confirmed (ADR 0009): no ownership transfers; a
  texture moves between queues with a fence if both queues support its
  layout.
- Copy queue: textures must be in `Common`, and copy queues can't perform
  layout transitions (verified against the Enhanced Barriers spec).
- Compute queue: `Common`, `ShaderRead`, `ShaderWrite`, `CopySource`,
  `CopyDest` (and `Undefined` as the layout before), the layouts D3D12
  compute lists support (corrected 2026-10-24; earlier text said "all other
  layouts"). Graphics: every layout. Validated.
- D3D12 queue-specific layouts (`DIRECT_QUEUE_*`, `COMPUTE_QUEUE_*`) cut;
  possible later optimization.

## Submit boundaries (decided 2026-10-24)

D3D12 "requires a full finish and flush of all GPU work at
`ExecuteCommandLists` completion" (Enhanced Barriers spec), and one Slag
submit is one `ExecuteCommandLists` call. So a submit starts after every
earlier submit on its queue has finished, with their writes visible, on
both backends: a resource last used in an earlier submit needs at most a
layout transition, from `syncBefore = None`. Within a submit, command
buffers are one scope; barriers order them as usual. Work on other queues
is ordered by semaphore waits, which use `ALL_COMMANDS` on Vulkan.

Vulkan has no such boundary (a barrier from `NONE` doesn't depend on
earlier submits), so each queue records one command buffer at
initialization holding a single global barrier (all commands, memory
writes → all commands, memory reads and writes) and puts it first in every
submit with command buffers. Tested by "a submit sees the finished work of
earlier submits on its queue", which synchronization validation fails
without it.

## GPU → CPU visibility (resolves ADR 0024's open question)

On Vulkan, Slag appends one global barrier at the end of every command
list: all commands / memory writes → host / host read. After a fence wait
the CPU sees GPU writes on both backends. No Host flag in the API.

## Cut

Split barriers / events; predication, stream output, shading rate, video,
ray tracing sync/access values (RT returns later);
`VK_KHR_unified_image_layouts`.

## Mixed read-only depth / writable stencil (proposed mapping)

The Enhanced Barriers spec defines no mixed layouts. `DEPTH_STENCIL_WRITE`
allows depth-stencil read access, and barriers can target individual planes.
Proposed: on D3D12, mixed cases use `DEPTH_STENCIL_WRITE` with a read-only
DSV variant; on Vulkan, `DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL` /
`DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL`. Confirm with the D3D12 debug
layer.

## Still to test

Layout required for a render-pass resolve destination on D3D12: not
documented publicly (`ResolveSubresource` uses `RESOLVE_DEST`). Test with the
debug layer.

## Planes (M2)

`SubresourceRange::plane = Color` on a depth/stencil texture covers every
plane (D3D12 `NumPlanes = 2`, Vulkan depth + stencil aspects); `Depth` and
`Stencil` select one.

## One Vulkan read-only layout (M3, 2026-10-24)

`VK_EXT_descriptor_heap` image descriptors carry the layout the image is in
when shaders read it. Slag maps both ShaderRead and DepthStencilRead to
`VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL` (core 1.3), so one texture descriptor
works in either layout, as on D3D12. Writable texture descriptors use
`GENERAL` (ShaderWrite).

## Mixed depth/stencil and resolves (confirmed 2026-10-24)

- Read-only depth with writable stencil (and the reverse): transition each
  plane to its layout (DepthStencilRead / DepthStencilWrite) and set the
  attachment's read-only flags. D3D12 binds the read-only DSV variant with
  `D3D12_RENDER_PASS_FLAG_BIND_READ_ONLY_DEPTH` / `_STENCIL`; Vulkan uses
  `DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL` /
  `DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL`. Clean on both validation
  layers.
- ~~A render-pass resolve destination stays in RenderTarget on both
  backends.~~ Wrong: with the D3D12 debug layer reporting it reliably (M7,
  2026-10-24), `EndRenderPass` needs the destination in `RESOLVE_DEST`
  (as Mesa's dzn also does), while Vulkan needs `COLOR_ATTACHMENT_OPTIMAL`,
  and D3D12 allows no barriers inside a render pass. **Render-pass resolves
  are cut** (owner decision, 2026-10-24): `StoreOp::Resolve` and
  `ColorAttachment::resolveView` are removed; MSAA is resolved with
  `CommandBuffer::resolveTexture` (ResolveSource → ResolveDest).

