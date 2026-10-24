# 0061 — Third-party dependencies

**Status:** Accepted (2026-10-23).

## Library dependencies (shipped)

Only two, both unavoidable:

| Dependency | Why |
|---|---|
| Vulkan-Headers | API definitions; header-only |
| DirectX 12 Agility SDK | D3D12 runtime DLL (Enhanced Barriers, Shader Model 6.8) and matching headers |

## Implemented in Slag instead of a library

| Instead of | Slag implements | Where |
|---|---|---|
| volk | Vulkan loader with per-device function tables (fits ADR 0001), from a hand-maintained function list | `src/vulkan/` |
| VMA and D3D12MA | One backend-agnostic sub-allocator for automatic placement, built on memory blocks and placed resources (ADR 0023); dedicated allocations for large targets; budget from DXGI / `VK_EXT_memory_budget`. Defragmentation designed in (ADR 0062) | `src/common/` + backends |
| SPIR-V reflection libraries | Minimal parser for vertex input semantics (ADR 0038) | `src/vulkan/` |
| A hashing library | 64-bit hash for D3D12 pipeline cache keys (ADR 0043) | `src/common/` |

## Sub-allocator policy (decided 2026-10-24)

- **Algorithm:** TLSF (two-level segregated fit): constant-time allocate and
  free, low fragmentation.
- **Blocks:** one pool per memory kind. Blocks are 256 MB, or heap size / 8
  on heaps of 1 GB or less. Blocks are memory blocks created internally
  (ADR 0023).
- **Dedicated allocations:** resources larger than half a block get their
  own memory (D3D12 committed resource, Vulkan dedicated allocation).
- **Freeing:** a block that becomes empty is released, except one empty
  block kept per memory kind to avoid churn.
- **Bookkeeping:** every allocation records owner, offset, size, and
  alignment, as defragmentation needs (ADR 0062).

## Tests, samples, and tools only (never linked into the library)

Allowed: a unit test framework, a windowing library (e.g. GLFW or SDL), and
an image loader (e.g. stb_image). Chosen so far: **doctest** for unit tests
(M0) and **SDL3** (3.4.18, zlib) for sample windows (M4, decided
2026-10-24). No image loader: sample textures are generated in code until a
sample needs image files.

## Tools

Slang (shader compiler), CMake, Ninja, clang-format.
