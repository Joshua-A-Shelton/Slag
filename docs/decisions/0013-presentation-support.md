# 0013 — Presentation support (instance and device level)

**Status:** Accepted (2026-10-22)

Swapchain behavior and accepted window types are decided in the swapchain
design. This note covers only instance/device setup.

## Decision

- **Vulkan instance:** always enable `VK_KHR_surface` and every available
  platform surface extension (`win32`, `xcb`, `xlib`, `wayland`). No flag. If
  none are available (headless), skip and log at Info.
- **Vulkan device:** enable `VK_KHR_swapchain` when available; otherwise
  swapchain creation fails later.
- **D3D12:** nothing needed beyond the DXGI factory.
- **No "can present" field** in graphics device info. Support is checked at
  swapchain creation, which returns an error if unsupported.
- **Present only from the graphics queue.**

## Deferred to the swapchain design

- `VK_EXT_surface_maintenance1` / `VK_EXT_swapchain_maintenance1`: required for
  swapchain creation (ADR 0056). Previously open: require,
  use when available, or skip.
- Linux window systems to accept: resolved by ADR 0055 (Xlib, XCB, Wayland).
