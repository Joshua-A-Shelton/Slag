# 0052 — Opaque pointers for all objects

**Status:** Accepted (2026-10-23). Supersedes ADR 0002. Names are
placeholders.

## Decision

Every Slag object is referenced by a **pointer to an opaque object**:
instance, graphics devices, queues, command lists, swapchains, buffers,
textures, attachment views, pipelines, memory blocks, query pools, fences.
There are no generational handles.

```cpp
Buffer*  buffer  = device->createBuffer(desc);
Texture* texture = device->createTexture2D(desc);
Semaphore* semaphore = device->createSemaphore(0);
// ...
device->destroy(buffer);
```

- Lifetime is explicit: create and destroy calls. After destroy the pointer
  is invalid.
- Each object records its device; using an object with another device is a
  validation error.
- Samplers are not objects; they are descriptors written into the sampler
  heap (ADR 0033).

## Why

One less indirection on every use, no handle table to synchronize on create
and destroy, simpler API.

## Given up

Generational handles detect use-after-free for free. Pointers can't
reliably: a stale pointer may point at freed or reused memory.

## Stale-pointer detection in validation builds (accepted)

- On destroy, Slag releases the native object and GPU memory immediately,
  but keeps its own small object record, marked destroyed, in a quarantine
  with a fixed size budget; the oldest records are freed when it fills.
- Every Slag call checks incoming pointers and reports "use of a destroyed
  <type>", with the debug name if there is one.
- Detection is very likely, not guaranteed: a pointer used long after its
  record left the quarantine may go undetected.
- Builds without validation free records immediately and pay nothing.
