# 0016 — Robustness and null descriptors

**Status:** Accepted (2026-10-22)

## Decision

- **Out-of-bounds access is undefined behavior** on every backend. Vulkan
  robustness features (`robustBufferAccess`, robustness2) stay **off** and
  are not exposed.
- **No null descriptors.** Accessing a heap slot that was never written (or
  whose resource was destroyed) is undefined behavior.
- In this area, Vulkan's behavior is the contract: D3D12 is made to match it
  as far as possible, not the other way around. This is an exception to the
  "D3D12 is the reference" rule (ADR 0015), chosen for speed.

## Why

Same behavior on all backends and no robustness cost. Shaders that rely on
D3D12's zero-on-out-of-bounds reads are bugs under this contract.

## D3D12 can't fully match

D3D12 hardware still returns 0 for out-of-bounds reads and null descriptors;
that is allowed under "undefined" but can hide bugs. Mitigations:

- **Detection:** Vulkan GPU-assisted validation catches out-of-bounds
  descriptor indexing and buffer access. D3D12 GPU-based validation catches
  accesses to uninitialized descriptors (verify coverage). Test on Vulkan
  with GPU-assisted validation to find code that depends on D3D12's zeros.
- **Poison descriptors (Accepted):** in validation builds on D3D12, fill
  unwritten and freed slots with descriptors pointing at a debug resource
  full of poison values, so bad reads show up as visibly wrong output
  instead of silent zeros. Scope (decided 2026-10-24, M7): **cleared slots
  and slots whose resource was destroyed**, with a poison view of the slot's
  last kind (buffer kind with its stride or format; texture view type and
  format class). Never-written slots stay null, since filling the whole heap
  at initialize is slow.
