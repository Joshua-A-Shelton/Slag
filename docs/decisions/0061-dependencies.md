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

## Tests, samples, and tools only (never linked into the library)

Allowed: a unit test framework, a windowing library (e.g. GLFW or SDL), and
an image loader (e.g. stb_image). Specific choices are made when the first
tests and samples are written.

## Tools

Slang (shader compiler), CMake, Ninja, clang-format.
