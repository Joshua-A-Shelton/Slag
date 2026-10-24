# Open items

Everything left unresolved after the first full pass of the interface design
(ADRs 0001–0059), updated after the spec and driver checks of 2026-10-23.
Resolve each one, record the outcome in the ADR it names, and remove it from
this list.

## Needs a decision from the project owner

None right now.

## Still to test (no public documentation found)

| # | Check | ADR |
|---|---|---|
| 12 | D3D12 GPU-based validation coverage for reads of uninitialized descriptors | 0016 |
| 13 | Oldest Slang version that emits descriptor-heap SPIR-V (2026.18.2 is confirmed to work) | 0034 |
| 14 | Intel Windows Vulkan driver support for `VK_EXT_descriptor_heap` | CLAUDE.md §3 |
| 15 | Whether every Enhanced Barriers device has resource heap tier 2 (it's a baseline check either way; this only affects coverage) | 0012, 0023 |
| 19 | Actual `imageDescriptorSize` / `bufferDescriptorSize` on current NVIDIA and Intel drivers (RADV measured: buffer 16, image 32 on RX 7900 XT / 64 on Raphael iGPU, sampler 16, reserved range 0). NVIDIA (RTX 2070 SUPER, driver 621.01) runs every heap test; sizes not yet logged | 0029 |
| 20 | AMD D3D12 driver supports Enhanced Barriers and Shader Model 6.8 on RDNA2 (RX 6000) — the fallback path for older AMD on Windows | CLAUDE.md §3 |
| 21 | Which GPU generations RADV's `VK_EXT_descriptor_heap` support covers | CLAUDE.md §3 |
| 16 | Verify the ◐ entries in `docs/format-support.md` against driver reports (plus its two other "to verify" items) | 0022 |
| 23 | `VK_EXT_descriptor_heap` address-range descriptors use the same range limits (`maxStorageBufferRange`, `maxTexelBufferElements`); Intel values | 0028 |
| 24 | Whether Windows on ARM (D3D12, Vulkan) or ARM64 Linux drivers meet the baseline | 0070 |

## Resolved on 2026-10-23

- `VK_EXT_descriptor_heap` entry points, heap limits (1,000,000 / 2048 fit),
  heap-switch cost → 0003, 0029.
- Resource heap default 262,144, maximum one line away via a constant → 0029.
- Older AMD GPUs on Windows use the D3D12 backend; revisit with the Vulkan
  1.3 fallback evaluation → CLAUDE.md §3, §14.
- CI split: hosted CI with WARP for D3D12; self-hosted GPU runner for Vulkan
  → CLAUDE.md §12.
- Maximum buffer size is a per-device value (named exception) → 0028.
- Present modes: switchable-modes query; others need `reconfigure` → 0056,
  0058.
- Sampler LOD bias constant lowered to ±14 (no baseline check) → 0033.
- Stale-pointer detection (quarantine) in validation builds → 0052.
- Vulkan Slang flags: `-capability spvDescriptorHeapEXT`,
  `-spirv-unified-descriptor-heap-stride` (shared slot index space) → 0034.
- Guaranteed format support table accepted; baseline and vertex formats
  updated → `docs/format-support.md`, 0012, 0038.
- Buffer view limits: 2^27 elements (typed, structured), 1 GiB per storage
  view → 0028.
- Slang 2026.18.2 compiles `ResourceDescriptorHeap[...]` to
  `SPV_EXT_descriptor_heap` SPIR-V that passes `spirv-val` (M0) → 0034.
- M1 spike on RADV: the heap slot stride Slag uses matches Slang's
  `-spirv-unified-descriptor-heap-stride` indexing, and `[[vk::push_constant]]`
  blocks are fed by `vkCmdPushDataEXT`, on both a 32-byte-image-descriptor
  GPU and a 64-byte one → 0029, 0034.
- All names settled: types, functions, structs, enums, constants, macros,
  files → docs/naming.md.
- Driver-reserved heap range exists; Slag hides it → 0032.
- Root constant buffer maps to push addresses; slot stays in v1 → 0044.
- Buffer descriptors are built from address ranges → 0025.
- Slang targets descriptor heaps with `-capability spvDescriptorHeapEXT` → 0034.
- `SV_StartInstanceLocation` needs Shader Model 6.8; baseline raised → 0012, 0047.
- Relaxed format casting is a separate D3D12 capability; added to baseline → 0012, 0022.
- Copy queues: textures in `Common`, no layout transitions → 0049.
- Raw buffer offset alignment: 256 bytes, no baseline check needed → 0030.
- Timestamp valid bits: per-device value (36–64) → 0051.
- `VK_KHR_present_wait`: patchy on Windows (absent on some Intel); frame
  latency stays an optional capability → 0057.

## Resolved on 2026-10-24

- WARP on the `windows-2025` GitHub runner passes the D3D12 baseline
  (Enhanced Barriers, Shader Model 6.8) and runs the M1 spike → CLAUDE.md §12.
- Sub-allocator: TLSF, 256 MB blocks (heap / 8 on small heaps), dedicated
  above half a block → 0061.

- ◐ format entries: met by RADV (RX 7900 XT, Raphael iGPU) and WARP;
  NVIDIA and Intel still unchecked (item 16 stays open for them).

## Resolved on 2026-10-24 (M3)

- Slang behavior on both targets: push constants, root constant buffer at
  `register(b1)` (with `-fvk-b-shift 0 0`), `SV_VertexID` / `SV_InstanceID`
  without bases, `SV_StartInstanceLocation`; no semantic names in SPIR-V,
  so vertex attributes go by location → 0034, 0038, 0044, 0047.
- D3D12 render-pass resolve destination: ~~the RenderTarget layout works
  (debug layer clean on WARP)~~ it needs `RESOLVE_DEST`, so render-pass
  resolves were cut in M7 → 0049.
- Mixed read-only depth / writable stencil: D3D12 uses the read-only DSV
  variant plus `BIND_READ_ONLY_DEPTH`; Vulkan
  `DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL`, with per-plane barriers →
  0049.

## Deferred past v1 (for reference, not blocking)

Mesh shaders and ray tracing (0005), native device import and creation hooks
(0018), sparse resources (0026), external memory / OpenXR (0027), transparent
windows (0056), the Vulkan 1.3 fallback (CLAUDE.md §3).
