# 0067 — How validation is built

**Status:** Accepted (2026-10-23). `ValidationFailureAction` is final
(docs/naming.md); internal names are implementation details.

## Two kinds, kept apart

| | Slag validation | Backend validation |
|---|---|---|
| What | Slag's own API-usage checks | D3D12 debug layer / GPU-based validation, Vulkan validation layers |
| Switch | `SLAG_VALIDATION` (compile time) | `ValidationFlags` in `InstanceDesc` (runtime, ADR 0010) |
| Cost when off | none (not compiled) | none (layers not loaded) |

In a `SLAG_VALIDATION` build, Slag's checks are **always on**: their
bookkeeping (descriptor slots, stale-pointer quarantine, command buffer
state) must run from the start. `ValidationFlags` only controls backend
layers.

## Where checks live

Every public function runs `SLAG_VALIDATE(...)` before dispatching (ADR
0065). Checks are written inline in that function, next to the call they
guard (decided 2026-10-24, M7); `src/common/validation/` holds the shared
bookkeeping: the stale-pointer quarantine and descriptor-slot tracking.
Without `SLAG_VALIDATION`
the macro and all bookkeeping compile to nothing.

## Failures

- Reported through the debug callback (ADR 0011) as `MessageSeverity::Error`,
  `MessageSource::Slag`. No separate handler. Messages name the function and
  the debug names of the objects involved, e.g.
  `slag: CommandBuffer::draw: no render pass is active (command buffer "MainPass")`.
  No file/line (the caller's location isn't known).
- `InstanceDesc` option:

  ```cpp
  enum class ValidationFailureAction : uint8_t { Break, Continue, Abort };
  ```

  `Break` (default) breaks into the debugger if attached, then continues.
- **The failing call still proceeds**, so validation and release builds do
  the same thing. Use `Abort` to stop at the first error.

## What is checked

- **Parameters:** create structs against limits, the format support table,
  per-type texture rules, alignments.
- **Lifetime:** stale pointers (ADR 0052), wrong-device objects, destroying
  objects Slag itself still references (a texture with live attachment
  views, a memory block with resources). **Not** destroying objects still
  used by in-flight GPU work: that would need tracking every resource each
  command buffer touches. `DeferredDestroyer` (ADR 0069) helps there.
- **Call order and state:** draws outside a render pass and disallowed work
  inside one, label balance, query resolve-before-reuse, split-pass
  submission, increasing semaphore values, resetting an in-flight command
  buffer, `reconfigure` preconditions.
- **Descriptors:** slot tracking and D3D12 poisoning (ADRs 0016, 0031).

## Not checked in v1

Texture layouts and barrier correctness. Covered by Vulkan synchronization
validation and D3D12 GPU-based validation; revisit later.

## Testing

Unit tests set `Continue`, capture messages through the debug callback, and
assert that each check fires.
