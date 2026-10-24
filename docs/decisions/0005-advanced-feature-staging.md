# 0005 — Mesh shaders, ray tracing, work graphs

**Status:** Accepted (2026-10-22)

## Decision

- **v1 core:** graphics (vertex + pixel), compute, indirect draw/dispatch,
  multiple queues.
- **v1.x, as optional capabilities:** mesh shaders, then ray tracing.
  Pipeline creation is designed now so these slot in without rework.
- **Work graphs:** not in the common interface (Vulkan has only an
  experimental vendor extension). Use the native escape hatch.

Which sub-features of each stage are exposed is decided per feature with the
project owner (see CLAUDE.md §13).
