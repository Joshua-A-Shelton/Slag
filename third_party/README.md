# Third-party dependencies

Pinned versions, vendored or fetched (CLAUDE.md §11). Nothing else in core
without discussion.

| Dependency | Used by |
|---|---|
| Vulkan-Headers | Vulkan backend |
| volk | Vulkan backend (loader, no link-time `vulkan-1`) |
| Vulkan Memory Allocator (VMA) | Vulkan backend |
| D3D12 Memory Allocator (D3D12MA) | D3D12 backend |
| DirectX 12 Agility SDK | D3D12 backend |

Not added yet: they come in with the first backend sources.
