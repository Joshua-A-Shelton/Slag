# 0056 — Swapchain creation options

**Status:** Accepted (2026-10-23). Names are placeholders.

## Formats

- `B8G8R8A8Unorm` (guaranteed), `R8G8B8A8Unorm`, `R10G10B10A2Unorm`,
  `R16G16B16A16Float`; `supportedSwapchainFormats(window)` reports the rest.
- Images are UNORM with the sRGB variant castable; attachment views choose
  sRGB (D3D12 flip model forbids sRGB image formats). Vulkan requires
  `VK_KHR_swapchain_mutable_format`.

## Size and image count

- Width/height are always explicit.
- Image count is a minimum (2 or 3); the actual count is read after creation.

## Present modes

| Slag | D3D12 | Vulkan |
|---|---|---|
| Fifo (always supported) | `Present(1, 0)` | `FIFO` |
| Mailbox (queried) | `Present(0, 0)` | `MAILBOX` |
| Immediate (queried) | `Present(0, ALLOW_TEARING)`; tearing flag set at creation when supported | `IMMEDIATE` |

- **The present mode can change on any present.** D3D12 picks it per
  `Present` call. Vulkan uses `VK_EXT_swapchain_maintenance1` (per-present
  mode, declared mode list at creation), which swapchain creation requires
  along with `VK_EXT_surface_maintenance1` (resolves ADR 0013's deferred
  extension question).
- Which modes can switch without recreating the swapchain is
  driver-specific (`VkSurfacePresentModeCompatibilityKHR`). Decided
  2026-10-23:
  - `swapchain->switchablePresentModes()` returns the modes the current
    swapchain can switch to on any present. D3D12: every supported mode.
    Vulkan: the driver's compatible set, declared at creation with
    `VkSwapchainPresentModesCreateInfoKHR`.
  - Presenting with a mode outside that set is a validation error; present
    returns an error without presenting.
  - Moving to a mode outside the set is done with `reconfigure`, which takes
    a present mode (ADR 0058). Slag never recreates the swapchain
    internally.
- Cut: `FIFO_RELAXED`.

## Other

- Usage: render target + copy destination.
- No scaling: swapchain size must match the window.
- **Opaque only in v1.** Transparent windows are deferred, to be revisited
  (tools and other non-game programs are valid use cases). On D3D12 they
  need a composition swapchain + DirectComposition target, with these
  trade-offs: hidden DComp objects and `Commit` calls; conflicts with other
  DComp users of the same window; the window must be created with
  `WS_EX_NOREDIRECTIONBITMAP`; tearing likely unavailable; possibly more
  compositor latency; a second presentation path to maintain. Vulkan:
  `compositeAlpha` support varies by window system.
- The swapchain provides a `Texture*` and an attachment view per image.

## Changing image count

After any present, through `reconfigure` (ADR 0058).
