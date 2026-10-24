# 0017 — Debug names

**Status:** Accepted (2026-10-22). Names are placeholders.

## Decision

- Every create struct has an optional `debugName` (`const char*`, UTF-8).
  Covers all objects: resources, pipelines, heaps, fences, queues, command
  lists, swapchains.
- No rename-after-creation function in v1.
- Names are applied **whenever given**, regardless of validation level, so
  they show up in capture tools (PIX, RenderDoc, Nsight). `nullptr` costs
  nothing. Naming happens once at creation, never on hot paths.
- Validation builds keep a copy of each name for Slag's own messages.
  Other builds keep none.

## Mapping

| | D3D12 | Vulkan |
|---|---|---|
| Set name | `ID3D12Object::SetName` (UTF-8 → UTF-16 in a fixed stack buffer, truncated at 255 characters, no heap allocation) | `vkSetDebugUtilsObjectNameEXT` |
| Requirement | none | `VK_EXT_debug_utils`, enabled whenever available |

Command-list labels/markers are decided in the command-list design.
