# 0045 — Command list allocation and reuse

**Status:** Accepted (2026-10-23). Names are placeholders.

## Decision

- **Each command list owns its allocator.** The user creates a command list
  for a queue type (graphics, compute, copy); internally it pairs one native
  list with its own allocator. No allocator object in the API.
- **The user resets it.** Cycle: `begin()` → record → submit → wait on a
  fence → `reset()` → record again. Resetting while the GPU still executes
  the list is a user error.
- **No internal recycling** and no fence tracking for correctness.
  Validation builds record the fence value at submit so a reset of an
  in-flight list is reported.
- **Threading:** one thread at a time per command list; different lists may
  record on different threads concurrently (§7).
- Command lists are pointers to opaque objects (ADR 0002).

## Mapping

| | D3D12 | Vulkan |
|---|---|---|
| Command list | `ID3D12GraphicsCommandList` + its own `ID3D12CommandAllocator` | `VkCommandBuffer` from its own `VkCommandPool` |
| Reset | `allocator->Reset()` + `list->Reset(allocator)` | `vkResetCommandPool` |

## Trade-off accepted

Each list keeps its own allocator memory, sized to the most it has ever
recorded. Shared allocators can be added later without breaking this
design if many small lists per frame make that matter.

## Cut

D3D12 bundles and Vulkan secondary command buffers (different semantics;
parallel recording uses multiple command lists).

## Not chosen

Allocator objects handing out short-lived lists; device-managed recycling
with internal fence tracking.
