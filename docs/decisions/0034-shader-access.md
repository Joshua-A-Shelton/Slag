# 0034 — Shader language and heap access

**Status:** Accepted (2026-10-23). Vulkan heap flags recorded; remaining
flags to be recorded.

## Decision

- **Slang** is the supported shader language and compiler, producing DXIL
  for D3D12 and SPIR-V for Vulkan from one source. HLSL via DXC and GLSL are
  not supported paths.
- Slang's **optional output flags** are used so both targets behave the
  same: matching CPU/GPU memory layout for constant and structured buffers,
  matching heap access, and push constants. The project owner has used this
  setup before; the exact flags will be recorded here and in CLAUDE.md, and
  later in a shader-compile helper.
- Shaders reach descriptors through the resource and sampler heaps by index
  (D3D12 Shader Model 6.6 heap indexing; Vulkan `VK_EXT_descriptor_heap`).
- Divergent indices must be marked non-uniform (`NonUniformResourceIndex`).
- Reading a slot as the wrong kind is undefined behavior; only
  GPU-assisted validation can catch it.
- No Slag shader header/module in v1; may be added later (e.g. typed index
  wrappers).

## Required Slang flags

| Target | Flag | Purpose |
|---|---|---|
| SPIR-V (Vulkan) | `-capability spvDescriptorHeapEXT` | Heap access through `VK_EXT_descriptor_heap` |
| SPIR-V (Vulkan) | `-spirv-unified-descriptor-heap-stride` | Index every descriptor kind with the largest descriptor stride, giving one shared slot index space (ADR 0029) |

Not yet known whether flags are needed for buffer layout, push constants,
semantic names in SPIR-V, `SV_VertexID`/`SV_InstanceID` base behavior,
start-instance mapping, or the root constant buffer register. These are
tested when shaders are first built; any flags found are added here.

## Slang and descriptor heaps (checked 2026-10-23)

- Slang targets descriptor heaps with `-capability spvDescriptorHeapEXT`
  (SPIR-V capability `DescriptorHeapEXT`, built-ins `ResourceHeapEXT` /
  `SamplerHeapEXT`, via `VK_KHR_shader_untyped_pointers`).
- A third-party project reports needing Slang 2026.14.1+ and SPIRV-Tools
  2026.3+. Not yet confirmed in Slang's own release notes.
- Shaders declaring `DescriptorHeapEXT` treat heap access as non-uniform by
  default; marking uniform access can help performance.

Accepted binary formats per pipeline (DXIL, SPIR-V, Shader Model 7
SPIR-V for D3D12): pipeline design.
