# 0020 — Buffers

**Status:** Accepted (2026-10-22). Names are placeholders.

## Decision

- **No usage flags.** Any buffer can be used for any purpose its memory
  kind allows.
  - Vulkan: Slag sets every relevant buffer usage bit.
  - D3D12: Slag sets `ALLOW_UNORDERED_ACCESS` on GPU-only and GPU-upload
    buffers (not allowed on upload/readback heaps).
- **Create struct:** size, memory kind (committed creation), debug name.
  Placed creation and size/alignment queries: see the creation item.
  Views are descriptors (ADR 0004).
- **Initial contents are undefined** on every backend. D3D12 always uses
  `D3D12_HEAP_FLAG_CREATE_NOT_ZEROED`. Users clear buffers themselves.
- **No initial state:** under Enhanced Barriers buffers have no layout.
- **Constant buffer alignment** is a fixed Slag constant of 256 bytes
  (D3D12 requirement; Vulkan `minUniformBufferOffsetAlignment` ≤ 256).

Ray tracing acceleration-structure buffers are decided in the RT design.
