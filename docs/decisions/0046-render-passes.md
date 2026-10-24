# 0046 — Render passes

**Status:** Accepted (2026-10-23). Names are placeholders.

`beginRendering(desc)` / `endRendering()`: D3D12 `BeginRenderPass` /
`EndRenderPass`; Vulkan `vkCmdBeginRendering` / `vkCmdEndRendering`.

## Per attachment

| Setting | Slag |
|---|---|
| Load | Load, Clear, Discard |
| Store | Store, Discard, Resolve |
| Clear value | float4 color; float depth + 8-bit stencil |
| Resolve | color average only, into another attachment view |
| Depth attachment | read-only depth flag, read-only stencil flag (ADR 0035) |

## Pass-wide

- No render area: always the full attachment (D3D12 has none); Vulkan
  `renderArea` is the full extent. Use scissors to restrict.
- Layer count derived from the attachment views.
- D3D12 `ALLOW_UAV_WRITES` is always set.
- **Splitting a pass across command lists** is supported: suspend at the end
  of one list, resume in the next (D3D12 `SUSPENDING_PASS` /
  `RESUMING_PASS`; Vulkan suspending/resuming flags). The lists must be
  submitted consecutively in the same submit (validated).

## Rules

- Attachments must already be in the render-target / depth-write /
  depth-read layout when the pass begins; users transition them with
  barriers.
- Inside a pass: graphics state, draws, debug labels, queries only. No
  barriers, copies, standalone clears, or dispatches.
- Draws outside a pass are not allowed.
- All validated.

## Cut

NoAccess/None load and store ops; depth, stencil, and integer resolves;
render area.
