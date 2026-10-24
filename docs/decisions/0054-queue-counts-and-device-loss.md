# 0054 — Queue counts and device loss

**Status:** Accepted (2026-10-23). Supersedes the "one queue of each type"
part of ADR 0009. Names are placeholders.

## Queue counts

- Device initialization takes a **queue count per type** (graphics,
  compute, copy), default 1 each.
- D3D12: creates the requested queues.
- Vulkan: separate hardware queues where available; beyond that, queues
  share hardware as in ADR 0009 (internal lock; deadlock rule).
- Queue priorities stay cut.

## Device loss

- `submit`, `waitFences`, `waitIdle`, and present return `DeviceLost`.
  `device->status()` can be queried at any time.
- The first detection triggers the crash report (ADR 0014).
- After loss, every call except destroy does nothing and returns
  `DeviceLost` (or an empty result).
- Recovery is the user's job: destroy everything on the device, then
  initialize a graphics device again.

| Detection | D3D12 | Vulkan |
|---|---|---|
| Reason / status | `GetDeviceRemovedReason` | `VK_ERROR_DEVICE_LOST` |
| Fences | values become `UINT64_MAX` | `VK_ERROR_DEVICE_LOST` from waits/queries |
| Present | `DXGI_ERROR_DEVICE_REMOVED` / `_RESET` | `VK_ERROR_DEVICE_LOST` |
