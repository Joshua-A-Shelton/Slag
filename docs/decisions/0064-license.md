# 0064 — License

**Status:** Accepted (2026-10-23).

## Decision

Slag is licensed under the **zlib license** (`LICENSE` at the repository
root).

- Commercial use, modification, static linking into closed products, and
  private (console) backends are all permitted.
- Shipped binaries need no attribution; source copies keep the notice, and
  altered source must be marked as modified.
- No explicit patent grant.

## Dependencies must stay compatible

- Vulkan-Headers (Apache-2.0 or MIT): compatible.
- DirectX 12 Agility SDK: Microsoft's own license, which allows
  redistributing its runtime DLL with applications. It is fetched at build
  time, never committed to this repository.
- Test, sample, and tool dependencies (ADR 0061) should be permissive
  (zlib, MIT, BSD, Boost, Apache-2.0, or public domain such as stb).

## Console backends

They live out of tree under their own (private) terms; the zlib license
doesn't require publishing them.
