# 0055 — Swapchain window input

**Status:** Accepted (2026-10-23). Names are placeholders. Resolves the
window-system question deferred by ADR 0013.

## Decision

```cpp
enum class WindowKind : uint8_t { Win32, Xlib, Xcb, Wayland };

struct NativeWindow {
    WindowKind kind;
    void*      display;   // Win32: HINSTANCE (optional), Xlib: Display*, Xcb: xcb_connection_t*, Wayland: wl_display*
    uint64_t   window;    // Win32: HWND, Xlib: Window, Xcb: xcb_window_t, Wayland: wl_surface*
};
```

| Kind | D3D12 | Vulkan |
|---|---|---|
| Win32 | `CreateSwapChainForHwnd` | `VK_KHR_win32_surface` |
| Xlib | error | `VK_KHR_xlib_surface` |
| Xcb | error | `VK_KHR_xcb_surface` |
| Wayland | error | `VK_KHR_wayland_surface` |

- Vulkan surfaces are created and destroyed with the swapchain; no surface
  object in the API. Recreation reuses the surface internally.
- Presentation support is checked at swapchain creation (ADR 0013).
- The window must outlive the swapchain. Slag doesn't watch window events.

## Cut

UWP / CoreWindow and composition swapchains, headless/offscreen swapchains,
Android, macOS, `VK_KHR_display`.
