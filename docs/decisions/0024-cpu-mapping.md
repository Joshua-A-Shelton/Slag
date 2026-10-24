# 0024 — CPU mapping

**Status:** Accepted (2026-10-22). Names are placeholders.

## Decision

- **Buffers only**, in the Upload, Readback, and GPU-upload memory kinds.
- **Persistently mapped:** mapped once at creation, unmapped at destruction.
  One function returns the CPU pointer; no map/unmap calls. A placed
  buffer's pointer is its memory block's mapping + offset (blocks are mapped
  once at creation).
- **No read/written ranges** (all CPU-visible memory is coherent, ADR 0019).

## Documented usage rules

- Upload and GPU-upload memory is write-combined: write only, sequentially;
  CPU reads are very slow.
- Readback memory is cached.
- CPU/GPU access to the same memory must be separated by fences. Slag does
  not track it.

## GPU→CPU visibility (resolved by ADR 0049)

D3D12: a fence wait suffices. Vulkan: a fence does not make device writes
visible to the host; a barrier with host-access destination is required.
Resolved: on Vulkan, Slag appends a host-visibility barrier at the end of
every command list (ADR 0049).

## Cut

Vulkan host image copy; mapping GPU-only memory on unified-memory devices.
