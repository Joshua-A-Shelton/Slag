# 0057 — Acquire and present

**Status:** Accepted (2026-10-23). Names are placeholders.

```cpp
uint32_t index;
Result r = swapchain->acquire(&index);   // Ok, Suboptimal, OutOfDate, DeviceLost
queue->submit({ .waits = ..., .lists = ..., .signals = ...,
                .acquiredSwapchains = {swapchain} });
r = queue->present(swapchain, PresentMode::Fifo);
```

## Acquire

- D3D12 `GetCurrentBackBufferIndex`; Vulkan `vkAcquireNextImageKHR` with an
  internal binary semaphore.
- Always use the returned index (Vulkan order isn't guaranteed).

## Waiting for the acquired image

- **Explicit:** `SubmitDesc::acquiredSwapchains` lists the swapchains whose
  newly acquired images the submit waits for (ADR 0053 gains this field).
  Vulkan adds the acquire semaphore wait to that submit; D3D12 ignores it.
- Validation reports a submit that uses an acquired image without listing
  its swapchain.

## Present

- `queue->present(swapchain, mode)` on a graphics queue; mode may change
  every present (ADR 0056).
- The swapchain create struct takes the queue it presents on (D3D12 ties a
  swapchain to one queue); presenting elsewhere is a validation error.
- D3D12: `IDXGISwapChain::Present`. Vulkan: an empty internal submit that
  signals the render-done binary semaphore, then `vkQueuePresentKHR`;
  semaphores are recycled with present fences (`swapchain_maintenance1`).
- Returns `Ok`, `Suboptimal`, `OutOfDate`, or `DeviceLost`.
- Vulkan acquire semaphores (fixed 2026-10-24): an image's acquire
  semaphore moves to its present slot at present and is reused only after
  that slot's present fence signals. The present waited for all earlier work
  on its queue, which the work that used the image is ordered before, so the
  submit that waited on the semaphore has finished. Recycling it when its
  image was acquired again, as before, wasn't enough: mailbox and immediate
  modes hand an image back after one frame, so acquiring before waiting for
  frames in flight reused a semaphore with a pending wait (seen on NVIDIA by
  the `render_graph` sample). Acquiring before or after the frame wait now
  both work; the pool grows when every semaphore is still in flight.

## User responsibilities

- Barrier the image to `Present` before presenting, and from
  `Undefined`/`Present` before rendering.
- Don't render into an image the GPU still uses (per-frame fences).

## Frame latency (optional capability)

`swapchain->setMaxFrameLatency(n)`, `swapchain->waitForNextFrame(timeout)`.
D3D12: `SetMaximumFrameLatency` + frame latency waitable object. Vulkan:
`VK_KHR_present_id` + `VK_KHR_present_wait`, waiting for frame N − n.
To verify: `present_wait` support on Windows Vulkan drivers.
