# 0038 — Vertex input

**Status:** Accepted (2026-10-23). Names are placeholders.

## Decision

- Graphics pipelines may declare an **input layout** whose attributes are
  identified by **semantic name + index** (`POSITION`, `TEXCOORD0`), as in
  D3D12.
- **D3D12:** matches semantics natively (`D3D12_INPUT_ELEMENT_DESC`).
- **Vulkan:** Slag reads the vertex shader's input interface from the SPIR-V
  to map each semantic to its location, then builds the binding/attribute
  descriptions.
- An empty input layout is allowed (vertex pulling).

## Shape

```cpp
enum class VertexStepRate : uint8_t { PerVertex, PerInstance };

struct VertexAttribute {
    const char* semantic;       // "POSITION", "TEXCOORD", ...
    uint32_t    semanticIndex;
    PixelFormat format;
    uint32_t    slot;
    uint32_t    offset;         // always explicit
};

struct VertexInputLayout {
    std::span<const VertexAttribute> attributes;
    std::span<const VertexStepRate>  slots;
};

struct VertexBufferBinding { Buffer* buffer; uint64_t offset, size; uint32_t stride; };
cmd->bindVertexBuffers(firstSlot, std::span<const VertexBufferBinding>);
```

## Rules

- **Stride is set at bind time** (D3D12 vertex buffer view; Vulkan dynamic
  vertex stride, core 1.3).
- **Instance step rate is 1 only.**
- **Explicit offsets only** (no `D3D12_APPEND_ALIGNED_ELEMENT`).
- **Limits:** 32 attributes, 32 slots (constants + baseline check).
- **Vertex formats** come from the `PixelFormat` enum, validated by the support
  table: 32-bit 1–4 channels Float/Uint/Sint; 16-bit 2 or 4 channels
  Float/Unorm/Snorm/Uint/Sint; 8-bit 2 or 4 channels Unorm/Snorm/Uint/Sint;
  R10G10B10A2 Unorm; R10G10B10A2 Uint only where `formatSupport()` reports
  it (not guaranteed on Vulkan; `docs/format-support.md`). `R32G32B32Float/Uint/Sint` are added to the enum
  as vertex-only formats. One-channel 8/16-bit vertex formats are not allowed.
- **Matching:** attributes the shader doesn't read are ignored; a shader
  input with no attribute fails creation with an error naming the semantic.
- Vertex pulling still works with an empty layout; base vertex/instance
  behavior of `SV_VertexID` / `SV_InstanceID` must be made consistent by the
  Slang flags.

## To verify

Plain SPIR-V carries locations, not semantic names. The compiler must emit
semantic decorations (`UserSemantic`, `SPV_GOOGLE_hlsl_functionality1`).
Confirm the Slang flag for it and add it to the required flag set.
