# 0001 — Backend dispatch

**Status:** Accepted (2026-10-22). Refined by ADR 0065 (object layout, generated table, registration).

## Decision

- Each device selects a per-backend **function table** at creation time.
  Public wrapper types forward through it.
- When exactly one backend is compiled in, a build-time switch makes the
  wrappers call the backend directly (inlinable, no indirection).
- No per-object vtables.

## Why

The indirect call is cheap next to the native driver call behind it, so
multi-backend builds (e.g. Windows with D3D12 + Vulkan) lose little. The
direct path matters for consoles and whole-program optimization, and keeps
the "no virtual dispatch on hot paths" goal honest for single-backend builds.
