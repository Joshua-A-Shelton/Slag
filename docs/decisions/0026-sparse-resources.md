# 0026 — Sparse (tiled) resources

**Status:** Accepted (2026-10-22)

## Decision

Not in v1. Possible later as an optional capability.

## Why

- Tile-mapping updates (`UpdateTileMappings`, `vkQueueBindSparse`) are slow
  on several drivers; many engines use software virtual texturing instead.
- Tile mapping is its own kind of queue operation, which would complicate
  the queue design.
- Memory reuse is already covered by placed resources and aliasing
  (ADR 0023).

## If added later

Sparse buffers and 2D textures, 64 KB standard tiles (D3D12 reserved
resources; Vulkan `residencyStandard2DBlockShape`), and shader residency
checks (`CheckAccessFullyMapped` / `shaderResourceResidency`) as a separate
capability.
