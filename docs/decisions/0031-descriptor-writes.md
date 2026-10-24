# 0031 — Writing descriptors

**Status:** Accepted (2026-10-23). Names are placeholders. Supersedes the
"copying slots" part of ADR 0004.

## Decision

- **One write function per descriptor kind** (e.g. `writeTexture(index,
  desc)`), each with a **batch form** taking a list of (index, description)
  pairs.
- Writes take effect **immediately** on both backends (D3D12 `Create*View`
  into the slot; Vulkan writes descriptor bytes into heap memory).
  - Writing a slot the GPU may be reading is a race; wait on a fence first.
  - Different slots may be written from different threads concurrently.
- **`clearDescriptor(index)`:** no-op without validation. In validation
  builds it marks the slot empty, and on D3D12 writes a poison descriptor
  (ADR 0016).
- **Slot tracking** in validation builds: each slot's kind and resource.
  Destroying a resource still referenced by slots is logged at **Info**
  (decided 2026-10-24, M7: apps usually overwrite slots rather than clear
  them, so a Warning would fire on almost every destroy), and those slots
  are poisoned on D3D12.
- **No descriptor copies** in v1.

## Why no copies

D3D12 `CopyDescriptorsSimple` expects sources in a non-shader-visible heap,
so copies would need a CPU-side shadow of the whole resource heap (memory
plus a second write per descriptor write). In a bindless design, writing a
descriptor again from its description is cheap and copies are rarely needed.
