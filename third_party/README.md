# Third-party dependencies

Pinned versions, vendored or fetched (CLAUDE.md §11). Nothing else in core
without discussion.

| Dependency | Used by |
|---|---|
| Vulkan-Headers | Vulkan backend |
| DirectX 12 Agility SDK | D3D12 backend |

Slag implements its own Vulkan loader and memory allocator instead of volk,
VMA, and D3D12MA (ADR 0061).

Tests and samples may add a unit test framework, a windowing library, and an
image loader; those never become library dependencies.

Not added yet: they come in with the first backend sources.
