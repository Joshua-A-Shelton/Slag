# 0042 — Render target formats, sample count, topology

**Status:** Accepted (2026-10-23). Names are placeholders.

Baked into graphics pipelines.

| Option | Slag | D3D12 | Vulkan |
|---|---|---|---|
| Color target formats | up to 8; `None` gaps allowed | `RTVFormats`, `NumRenderTargets` | `VkPipelineRenderingCreateInfo` |
| Depth/stencil format | one, optional | `DSVFormat` | depth + stencil formats (filled from the one format) |
| Sample count | 1, 2, 4, 8; shared by all targets | `SampleDesc.Count` | `rasterizationSamples` |
| Sample mask | `uint32_t`, default all bits | `SampleMask` | `pSampleMask` |
| Topology | PointList, LineList, LineStrip, TriangleList, TriangleStrip | type in PSO + `IASetPrimitiveTopology` at bind | `topology` |

- Formats must match the bound attachment views exactly (the view's format,
  which may be a castable format); mismatch is a validation error.

## Cut

Adjacency topologies, patch lists, triangle fans, multiview / view
instancing (could return with VR support).
