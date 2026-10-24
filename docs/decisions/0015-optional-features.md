# 0015 — Optional features

**Status:** Accepted (2026-10-22)

## Decision

- **No opt-in.** Every optional feature Slag exposes is enabled
  automatically when the device supports it. Users check capabilities
  before using a feature; using an unsupported one is a validation error.
- **D3D12's feature set is the reference.** Slag only exposes optional
  features that exist in D3D12 (and that Vulkan can map). On Vulkan, Slag
  enables only the features needed to implement those. Vulkan-only features
  (e.g. ASTC/ETC2 compression, wide lines) are not exposed or enabled.
- A feature that costs performance merely by being enabled is not exposed.
  If one is ever worth exposing, it gets its own explicit request field as a
  documented exception.
- Initializing a graphics device takes no feature options (its only options
  are descriptor heap sizes, ADR 0029, the pipeline cache blob, ADR 0043,
  and queue counts, ADR 0054).

## Capabilities

- Queried on the **listed** graphics device, before initialization. D3D12
  collects them with the device Slag already creates for the baseline check
  (ADR 0012); Vulkan uses property queries.
- A plain struct of bools and small enums. Each D3D12 tier / Vulkan feature
  mapping is decided when that feature is added.
- Numeric limits are decided with the resource design.
