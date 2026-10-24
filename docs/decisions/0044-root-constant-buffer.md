# 0044 — Root constant buffer slot

**Status:** Accepted (2026-10-23). Vulkan mapping verified. Names are
placeholders.

## Decision

The fixed pipeline layout (ADR 0003) gains **one constant buffer slot set
by address and size** while recording (e.g. `cmd->setConstantBuffer(address,
size)`).

- **D3D12:** a root CBV (2 DWORDs; with 128 bytes of push constants the root
  signature uses 34 of 64 DWORDs).
- **Vulkan:** the buffer's address goes in push data, and the shader's
  constant buffer binding is mapped to it with
  `VK_DESCRIPTOR_MAPPING_SOURCE_PUSH_ADDRESS_EXT`
  (`VkShaderDescriptorSetAndBindingMappingInfoEXT` at pipeline creation).
  Push descriptors are not used; they are mutually exclusive with push
  data.
- Size is required: Vulkan needs a range, and validation checks it against
  the 64 KB constant buffer limit. Address alignment is 256 bytes.
- No bounds checking (D3D12 root CBVs have none); out-of-bounds reads are
  undefined (ADR 0016).
- Shaders reach it at a fixed register mapped by the Slang flags.

## Why

Per-pass/per-draw constants from a per-frame ring buffer without writing a
descriptor per allocation.

## Vulkan check (resolved 2026-10-23)

`VK_EXT_descriptor_heap` maps it cleanly through push addresses (the
extension's documented replacement for D3D12 root CBVs). The slot stays in
v1. Out-of-bounds access through it is invalid, consistent with ADR 0016.
