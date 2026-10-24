# 0059 — Fullscreen, tearing, HDR

**Status:** Accepted (2026-10-23). Names are placeholders.

## Fullscreen

- No fullscreen API: fullscreen is a borderless window covering the screen,
  managed by the user's windowing code.
- Exclusive fullscreen is cut (DXGI `SetFullscreenState`,
  `VK_EXT_full_screen_exclusive`).
- D3D12: Slag calls `MakeWindowAssociation(DXGI_MWA_NO_ALT_ENTER)`.

## Tearing and VRR

- Immediate mode is the tearing mode; on Windows it only tears in a
  borderless window covering the screen (documented).
- Variable refresh rate is driver-handled; nothing in the API.

## HDR

| Color space | Format | D3D12 | Vulkan |
|---|---|---|---|
| SrgbNonlinear (default) | any | `G22_NONE_P709` | `SRGB_NONLINEAR_KHR` |
| Hdr10 | `R10G10B10A2Unorm` | `G2084_NONE_P2020` | `HDR10_ST2084_EXT` |
| ScRgbLinear | `R16G16B16A16Float` | `G10_NONE_P709` | `EXTENDED_SRGB_LINEAR_EXT` |

- Chosen at creation; changeable with `reconfigure`.
- `supportedSwapchainFormats(window)` returns format + color space pairs
  (D3D12 `CheckColorSpaceSupport`; Vulkan surface formats).
- `VK_EXT_swapchain_colorspace` enabled whenever available.
- `swapchain->setHdrMetadata(desc)`: primaries, white point, min/max
  mastering luminance, MaxCLL, MaxFALL. D3D12 `SetHDRMetaData`; Vulkan
  `VK_EXT_hdr_metadata` (skipped + Info log when missing).

## Cut

Exclusive fullscreen; display luminance queries (D3D12
`IDXGIOutput6::GetDesc1` has no portable Vulkan equivalent).
