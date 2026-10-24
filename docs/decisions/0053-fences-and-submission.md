# 0053 — Fences and submission

**Status:** Accepted (2026-10-23). Names are placeholders.

## Fences

Timeline semaphores (`Semaphore*`, ADR 0052; called fences in this note) are the only sync primitive.

| Slag | D3D12 (`ID3D12Fence`) | Vulkan (timeline semaphore) |
|---|---|---|
| `createSemaphore(initialValue)` | `CreateFence` | `vkCreateSemaphore` (timeline) |
| `semaphore->value()` | `GetCompletedValue` | `vkGetSemaphoreCounterValue` |
| `semaphore->signal(value)` (CPU) | `ID3D12Fence::Signal` | `vkSignalSemaphore` |
| `semaphore->wait(value, timeoutNs)` (CPU) | `ID3D12Fence::SetEventOnCompletion` + `WaitForSingleObject` | `vkWaitSemaphores` |
| `slag::waitSemaphores(span<SemaphoreValue>, WaitAll \| WaitAny, timeoutNs)` (CPU) | `SetEventOnMultipleFenceCompletion` + `WaitForSingleObject` | `vkWaitSemaphores` |

- Values must **strictly increase** (validated).
- CPU waits live on the semaphores, not the device (changed after M8,
  2026-10-24). Both native waits need the device, but every semaphore knows
  its own, so `waitSemaphores` takes it from them; mixing devices in one wait
  is a validation error.
- Timeouts in nanoseconds; D3D12 rounds up to milliseconds.
- Cut: cross-process shared fences.

## Submission

```cpp
struct SemaphoreValue { Semaphore* semaphore; uint64_t value; };
struct SubmitDesc {
    std::span<const SemaphoreValue> waits;
    std::span<CommandBuffer* const> commandBuffers;
    std::span<const SemaphoreValue> signals;
    std::span<Swapchain* const>     acquiredSwapchains;  // ADR 0057
};
Result queue->submit(const SubmitDesc&);
```

- D3D12: `Wait` per wait → `ExecuteCommandLists` → `Signal` per signal.
  Vulkan: one `vkQueueSubmit2`.
- No stage mask on waits: a wait holds back all of the submit's work
  (Vulkan uses all-commands).
- Wait-before-signal is allowed; the shared-queue deadlock rule from ADR
  0009 applies.
- Split render pass lists go consecutively in one `lists` span (ADR 0046).
- One thread at a time per queue (§7), except the internal lock for a shared
  `VkQueue` (ADR 0009).
- Device loss is reported through the returned `Result`.

## Idle

`queue->waitIdle()` and `device->waitIdle()` (D3D12: internal fence signal
+ wait; Vulkan: `vkQueueWaitIdle` / `vkDeviceWaitIdle`), for shutdown and
debugging.
