# 0008 — Instance and graphics devices

**Status:** Accepted (2026-10-22). Names are placeholders.

## Decision

- The user creates an **instance** for one backend. The instance description
  holds the backend tag, options common to all backends, and the
  backend-options union (ADR 0010).
- The instance lists the **graphics devices** it can use. Each graphics
  device is one object for its whole life, in two states:
  1. **Listed:** carries info only (name, vendor/device ID, type, memory
     sizes, LUID, driver version string). Costs nothing beyond enumeration.
  2. **Initialized:** after an explicit initialize call, the same object
     creates resources and owns its queues. Several graphics devices may be
     initialized at once.
- There is no separate adapter object.

## Graphics device info

| Field | D3D12 | Vulkan |
|---|---|---|
| Name | `DXGI_ADAPTER_DESC3::Description` | `deviceName` |
| Vendor / device ID | `VendorId` / `DeviceId` | `vendorID` / `deviceID` |
| Type: Discrete / Integrated / Software | software flag + UMA query | `deviceType` (Virtual/Other not exposed) |
| Dedicated memory | `DedicatedVideoMemory` | sum of `DEVICE_LOCAL` heaps |
| Shared memory | `SharedSystemMemory` | sum of other heaps |
| LUID | `AdapterLuid` | `deviceLUID` |
| Driver version (string, logging only) | UMD version | `driverInfo` |

Not exposed: UUID, subsystem ID/revision, multi-GPU nodes/device groups.

## Ordering

Listed in preference order (high performance or low power). D3D12 uses
`EnumAdapterByGpuPreference`; Vulkan is sorted by Slag (discrete, then
dedicated memory, then integrated).

## Related

Filtering of devices below the baseline: ADR 0012. Capability queries:
ADR 0015.
