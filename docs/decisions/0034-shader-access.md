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
| SPIR-V (Vulkan) | `-fvk-b-shift 0 0` | Keep D3D `b` register numbers as Vulkan bindings in set 0, so the root constant buffer at `register(b1)` is binding 1 (ADR 0044); without it Slang warns but assigns the same binding |

Not yet known whether flags are needed for buffer layout, push constants,
semantic names in SPIR-V, `SV_VertexID`/`SV_InstanceID` base behavior,
start-instance mapping, or the root constant buffer register. These are
tested when shaders are first built; any flags found are added here.

## Confirmed in M0

Slang 2026.18.2 compiles `ResourceDescriptorHeap[index]` with the flags
above to SPIR-V using `DescriptorHeapEXT`, `ResourceHeapEXT`, and
`SPV_KHR_untyped_pointers`, which `spirv-val --target-env vulkan1.4` accepts.
DXIL output needs DXC, so it is only built on Windows.

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

## Confirmed in M3 (2026-10-24)

- `[[vk::push_constant]] ConstantBuffer<T>` becomes root constants at `b0`
  on D3D12 and push data on Vulkan.
- The root constant buffer is `ConstantBuffer<T> name : register(b1);`.
- Vertex inputs carry `Location` decorations only, no semantic names, so
  vertex attributes are matched by location (ADR 0038).
- `SV_VertexID` / `SV_InstanceID` keep D3D12 values (no base vertex or
  instance) on Vulkan: Slang subtracts `BaseVertex` / `BaseInstance`, which
  needs `shaderDrawParameters` (ADR 0012). `SV_StartInstanceLocation` reads
  `BaseInstance`.

## Buffer layout (2026-10-28)

`-fvk-use-dx-layout` gives SPIR-V the layouts D3D12 uses: constant-buffer
packing rules, and packed structured buffers (a struct of a `float3` and a
`float` has a 16-byte stride; a 104-byte struct keeps its 104-byte stride).
Packed strides need Vulkan's `scalarBlockLayout`, which Vulkan 1.4 makes
mandatory; Slag enables it on every device (ADR 0012). Found by Crucible's
shader toolchain (its ADR 0047), which builds with this flag.

