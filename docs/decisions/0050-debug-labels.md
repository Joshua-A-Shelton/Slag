# 0050 — Debug labels

**Status:** Accepted (2026-10-23). Names are final (docs/naming.md; settled after M7).

## Decision

- Command-list labels: `beginLabel(name, color)`, `endLabel()`,
  `insertLabel(name, color)`.
  - D3D12: `BeginEvent` / `EndEvent` / `SetMarker`.
  - Vulkan: `vkCmdBeginDebugUtilsLabelEXT` / `End` / `Insert`.
- **Color is optional and best effort:** Vulkan uses it; D3D12 ignores it in
  v1 (plain `BeginEvent` has no color). PIX event encoding could be added
  later without a dependency.
- **Text:** UTF-8; D3D12 converts to UTF-16 in a fixed stack buffer,
  truncated at 255 characters (as ADR 0017).
- **Balanced within one command list** (validated), except a label may span
  the lists of a split render pass (ADR 0046).
- **Always active**, like debug names. Skipped on Vulkan when
  `VK_EXT_debug_utils` is unavailable. A compile-time switch
  (e.g. `SLAG_DISABLE_LABELS`) strips them.
- With crash diagnostics on, D3D12 reports show open labels next to
  breadcrumbs (ADR 0014). Vulkan reports don't include labels.

## Cut

Queue-level labels.
