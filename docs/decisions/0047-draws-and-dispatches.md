# 0047 — Draws and dispatches

**Status:** Accepted (2026-10-23). Names are placeholders.

## Direct

| Slag | D3D12 | Vulkan |
|---|---|---|
| `draw(vertexCount, instanceCount, firstVertex, firstInstance)` | `DrawInstanced` | `vkCmdDraw` |
| `drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance)` | `DrawIndexedInstanced` | `vkCmdDrawIndexed` |
| `dispatch(x, y, z)` | `Dispatch` | `vkCmdDispatch` |
| `bindIndexBuffer(buffer, offset, size, Uint16 \| Uint32)` | `IASetIndexBuffer` | `vkCmdBindIndexBuffer2` |

## Indirect

- Draw, indexed draw, and dispatch; argument structs match both APIs'
  layouts (`D3D12_DRAW_ARGUMENTS` = `VkDrawIndirectCommand`, etc.).
- Each call: argument buffer (buffer + offset), max count, optional count
  buffer (buffer + offset).
- D3D12: `ExecuteIndirect` with internal command signatures (one per kind).
  Vulkan: `vkCmdDraw*Indirect`, `vkCmdDraw*IndirectCount`,
  `vkCmdDispatchIndirect`. `multiDrawIndirect` and `drawIndirectCount` in
  the Vulkan baseline.
- Arguments are tightly packed (no custom stride).

## Draw ID

Convention: the user writes each draw's index into its `firstInstance`;
shaders read the start-instance value (D3D12 `SV_StartInstanceLocation`,
Vulkan `BaseInstance`, mapped by Slang). This needs **Shader Model 6.8** on
D3D12: `SV_StartInstanceLocation` is mandatory in Shader Model 6.8, so the
D3D12 baseline is raised to **Shader Model 6.8** (verified 2026-10-23; recent
NVIDIA and AMD drivers report 6.8).

## Cut

8-bit indices, dispatch base group offset, custom indirect stride,
per-draw constant/binding changes in indirect calls (D3D12 signature
arguments / Vulkan device-generated commands).
