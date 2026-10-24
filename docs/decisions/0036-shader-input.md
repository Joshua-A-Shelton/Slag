# 0036 — Shader input to pipelines

**Status:** Accepted (2026-10-23). Names are placeholders.

## Decision

- **No shader objects.** Each pipeline create struct takes, per stage, the
  bytecode and an entry point name.
  - D3D12: `D3D12_SHADER_BYTECODE` per stage in the pipeline state stream.
  - Vulkan 1.4: SPIR-V passed directly in the stage create info
    (maintenance5); no `VkShaderModule`.
- **Entry points:** every stage names one. On D3D12 a DXIL blob must
  contain exactly one entry point (validated).
- **Light reflection only.** No binding reflection (the pipeline layout is
  fixed, ADR 0003), but Slag may read a shader's input signature, e.g. to
  match vertex attributes by semantic name on Vulkan (ADR 0038). Keep any
  reflection minimal.
- Bytecode only has to stay alive for the duration of the create call.

## Bytecode selection: list of tagged binaries

```cpp
enum class ShaderFormat : uint8_t {
    Dxil,
    Spirv,
    // 0x80 and up reserved for external (console) backends,
    // defined in their private headers
};

struct ShaderBinary {
    ShaderFormat format;
    const void*  data;
    size_t       size;
};

struct ShaderStage {
    std::span<const ShaderBinary> binaries;
    const char* entryPoint;
};
```

- Slag uses the **first binary in the list the backend accepts**. Today:
  D3D12 accepts `Dxil`, Vulkan accepts `Spirv`. With Shader Model 7, D3D12
  may also accept SPIR-V; list order expresses preference.
- If none is accepted, creation fails and the error lists the offered formats.
- Consoles add enum values only, never struct fields.
- If D3D12 SPIR-V needs different compiler flags than Vulkan SPIR-V, it gets
  its own format value.

## Not chosen

A single blob for the active backend (backend checks in app code), and one
field per backend (struct grows per backend; ambiguous under Shader Model 7).
