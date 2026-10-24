# Third-party dependencies

Pinned versions, vendored or fetched (CLAUDE.md §11). Nothing else in core
without discussion.

| Dependency | Version | Used by |
|---|---|---|
| Vulkan-Headers | v1.4.366 | Vulkan backend |
| DirectX 12 Agility SDK | 1.619.6 (retail) | D3D12 backend |
| doctest | v2.5.3 | unit tests only |

All are fetched at configure time by `cmake/SlagDependencies.cmake`, pinned by
version and SHA-256; nothing is committed here. For offline builds, set
`FETCHCONTENT_SOURCE_DIR_<NAME>` to a local copy.

Slag implements its own Vulkan loader and memory allocator instead of volk,
VMA, and D3D12MA (ADR 0061). Samples may later add a windowing library and an
image loader; those never become library dependencies.
