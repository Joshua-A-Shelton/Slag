# 0007 — No C API (for now)

**Status:** Accepted (2026-10-22)

## Decision

No C API in v1. The public C++ API stays C-shaped — plain structs, no
templates, no `std::function` — so a C layer can be written or generated
later at low cost.
