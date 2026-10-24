# 0043 — Pipeline caching and thread-safe creation

**Status:** Accepted (2026-10-23). Names are placeholders.

## Decision

- **One opaque cache blob per device.**
  - Optional input at device initialization (a span of bytes saved from a
    previous run).
  - Readable at any time as a byte buffer, for the user to save.
  - A blob from a different driver/GPU is discarded and logged at Info; the
    device starts with an empty cache.
- **Pipeline creation is thread-safe.** No async API and no Slag thread
  pool; users create pipelines on their own worker threads.

## Mapping

| | D3D12 | Vulkan |
|---|---|---|
| Cache | `ID3D12PipelineLibrary`, keyed by a Slag hash of the pipeline description + bytecode | `VkPipelineCache` (internally synchronized) |
| Save | `Serialize` | `vkGetPipelineCacheData` |
| Mismatched blob | driver-mismatch error on load → discard | header check → discard |

## D3D12 cache keys (decided 2026-10-23)

`ID3D12PipelineLibrary` stores pipelines under a name. Slag derives the name
from a 64-bit hash of everything that defines the pipeline: shader
bytecode, entry points, vertex layout, rasterizer / depth-stencil / blend
state, topology, target formats, and sample count. The hash is a small
function inside Slag (no dependency, ADR 0061). A collision is harmless:
D3D12 rejects a stored pipeline whose description doesn't match exactly,
and Slag then compiles normally. Vulkan needs no key (`VkPipelineCache`
matches internally).

## Cut

Per-pipeline blobs; `FAIL_ON_PIPELINE_COMPILE_REQUIRED` (no D3D12
equivalent).
