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
- **Exception (M7):** a call returns right after its report where going on
  would corrupt Slag's own state rather than hand invalid input to the
  driver: destroying an object again (it would be freed twice), destroying
  another device's object, creating on a device that isn't initialized,
  initializing twice, waiting on a null semaphore, and reconfiguring a
  destroyed swapchain.

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

Implemented in M7 (`tests/validation/`, decided 2026-10-24): Slag's common
code is built a second time, always with validation, around a **null test
backend** that only creates plain Slag objects, so every check can be
triggered without a GPU, on hosted CI too. In that build each failing check
records its source line; the `validation_coverage` test runs every
validation test, finds every check in `src/common/` and `src/util/`, and
fails if one never fired (398 of 398 at the end of M7). Writing the tests
found one bug: `resolveTexture` never rejected integer or depth formats
(`hasTraits` with two flags needs both).

Not built: the "resetting an in-flight command buffer" check listed above.
Slag can't know when GPU work finishes without tracking each submit's
semaphore values; the Vulkan validation layers and the D3D12 debug layer
report it.
