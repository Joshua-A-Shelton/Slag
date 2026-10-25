# 0068 — Per-call thread safety

**Status:** Accepted (2026-10-23).

## General rule

- Calls on **different objects** may run concurrently.
- Calls on **the same object** are externally synchronized (one thread at a
  time) unless listed as thread-safe below.
- Objects that don't change after creation (buffers, textures, attachment
  views, pipelines, memory blocks, query pools) may be **used** by many
  command buffers on different threads at once.

## Per object

| Object | Thread-safe | One thread at a time (on that object) |
|---|---|---|
| `Instance` | `graphicsDevices()` | `slag::destroy(instance)`: nothing else may use the instance |
| `GraphicsDevice` | every `create*` and `destroy()`; `info()`, `capabilities()`, `formatSupport()`, `placementInfo()`, `copyFootprint()`, `memoryBudget()`, `status()`, `pipelineCacheData()`; descriptor writes/clears to **different** slots | `initialize()` / `shutdown()`: nothing else may use the device; `waitIdle()`: no submits or presents may run concurrently |
| `CommandQueue` | `timestampFrequency()` | `submit()`, `present()`, `waitIdle()` |
| `CommandBuffer` | — | everything |
| `Swapchain` | `imageCount()`, `switchablePresentModes()` | `acquire()`, `reconfigure()`, `setHdrMetadata()`, `setMaxFrameLatency()`, `waitForNextFrame()` |
| `Semaphore` | `value()`, `wait()`, and the free `waitSemaphores()` | `signal()` |
| `Buffer` | `gpuAddress()`, `mappedData()` (CPU access to the memory is the user's to synchronize) | — |
| Every object | `userData()` while no `setUserData()` runs | `setUserData()` |
| Defragmentation pass | — | the whole pass; from begin to end no thread may use a moved resource (`gpuAddress()` and `mappedData()` included) or destroy it (ADR 0062) |

Callbacks may be invoked from any thread, concurrently: the debug callback
(ADR 0011) and `AllocatorCallbacks` (ADR 0066).

## Stricter than D3D12 (because of Vulkan)

- `device->waitIdle()` excludes concurrent submits/presents
  (`vkDeviceWaitIdle` requires external synchronization of all queues).
- `semaphore->signal()` is one thread at a time.

## Slag's internal locks

- Create/destroy: each object pool, the GPU memory allocator, the
  stale-pointer quarantine.
- Pipeline creation: the pipeline cache index.
- Submit/present, only when queues share a `VkQueue`: the shared-queue lock
  (ADR 0009).
- Attachment view create/destroy: the device's view-list lock, which keeps
  each texture's views for defragmentation to rebuild (ADR 0062).
- Validation builds only: descriptor-slot tracking. Slot records are
  atomics; the list of slots a defragmentation pass left stale has a lock,
  which submits take only while the list is non-empty.

No locks on recording, descriptor writes, or submits to separate hardware
queues.
