# 0002 — Handle style

**Status:** Superseded by ADR 0052 (opaque pointers for all objects).

## Decision

Two kinds of object references:

- **Numerous, short-lived objects** (buffers, textures, samplers, pipelines,
  attachment views): small typed **generational handles** (index +
  generation). Trivially copyable; debug builds detect use-after-free and
  cross-device misuse.
- **Few, long-lived objects used on hot paths** (device, queues, command
  lists, swapchain): **pointers to opaque objects**, so recording calls never
  pay a handle lookup.

## Why

Handles are cheap to store in user data structures and give validation for
free; command lists are called thousands of times per frame and should be a
direct pointer.
