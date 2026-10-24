#pragma once

// Swapchain, NativeWindow, PresentMode, ColorSpace, HDR metadata (ADRs 0055–0059).

#include <slag/PixelFormat.h>
#include <slag/Result.h>

#include <cstdint>
#include <span>

namespace slag {

class AttachmentView;
class CommandQueue;
class Texture;

enum class WindowKind : uint8_t
{
    Win32,
    Xlib,
    Xcb,
    Wayland,
};

// The window to present to (ADR 0055). It must outlive the swapchain; Slag
// doesn't watch window events.
struct NativeWindow
{
    WindowKind kind = WindowKind::Win32;
    void* display =
        nullptr;         // Win32: HINSTANCE (optional), Xlib: Display*, Xcb: xcb_connection_t*, Wayland: wl_display*
    uint64_t window = 0; // Win32: HWND, Xlib: Window, Xcb: xcb_window_t, Wayland: wl_surface*
};

// Fifo is always supported; Mailbox and Immediate (tearing) are queried
// through supportedPresentModes / switchablePresentModes (ADR 0056).
enum class PresentMode : uint8_t
{
    Fifo,
    Mailbox,
    Immediate,
};

// ADR 0059. Hdr10 needs R10G10B10A2Unorm, ScRgbLinear R16G16B16A16Float.
enum class ColorSpace : uint8_t
{
    SrgbNonlinear,
    Hdr10,
    ScRgbLinear,
};

struct SwapchainFormat
{
    PixelFormat format = PixelFormat::B8G8R8A8Unorm;
    ColorSpace colorSpace = ColorSpace::SrgbNonlinear;
};

// Images are render targets and copy destinations. 8-bit formats also get
// their sRGB variant as a castable format, for sRGB attachment views
// (ADR 0056).
struct SwapchainDesc
{
    NativeWindow window;
    CommandQueue* queue = nullptr; // a graphics queue; the only one it presents on
    uint32_t width = 0;            // always explicit; must match the window
    uint32_t height = 0;
    uint32_t minImageCount = 3; // 2 or 3; imageCount() gives the actual count
    PixelFormat format = PixelFormat::B8G8R8A8Unorm;
    ColorSpace colorSpace = ColorSpace::SrgbNonlinear;
    PresentMode presentMode = PresentMode::Fifo; // also decides which modes present() can switch to
    const char* debugName = nullptr;
    uint64_t userData = 0;
};

// Everything that can change after creation (ADR 0058).
struct SwapchainReconfigureDesc
{
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t minImageCount = 3;
    PixelFormat format = PixelFormat::B8G8R8A8Unorm;
    ColorSpace colorSpace = ColorSpace::SrgbNonlinear;
    PresentMode presentMode = PresentMode::Fifo;
};

// Chromaticities in CIE 1931 xy; luminance in nits (ADR 0059).
struct HdrMetadata
{
    float redPrimary[2] = {};
    float greenPrimary[2] = {};
    float bluePrimary[2] = {};
    float whitePoint[2] = {};
    float maxMasteringLuminance = 0.0f;
    float minMasteringLuminance = 0.0f;
    float maxContentLightLevel = 0.0f;
    float maxFrameAverageLightLevel = 0.0f;
};

class Swapchain
{
public:
    // The image to render next: Ok, Suboptimal (reconfigure soon), OutOfDate
    // (reconfigure first), DeviceLost. The submit that uses the image lists
    // this swapchain in SubmitDesc::acquiredSwapchains (ADR 0057).
    Result<uint32_t> acquire();

    uint32_t imageCount() const;
    // Valid until the next reconfigure or destroy.
    Texture* texture(uint32_t index) const;
    AttachmentView* attachmentView(uint32_t index) const;
    // The sRGB view of an 8-bit format's image.
    AttachmentView* srgbAttachmentView(uint32_t index) const;

    // Modes present() can use without reconfiguring (ADR 0056).
    std::span<const PresentMode> switchablePresentModes() const;

    // The GPU must be done with every image and none may be acquired but not
    // presented (ADR 0058). Earlier texture and view pointers become invalid.
    Result<void> reconfigure(const SwapchainReconfigureDesc& desc);

    // Skipped (and logged) where the platform has no HDR metadata support.
    void setHdrMetadata(const HdrMetadata& metadata);

    // Needs GraphicsDeviceCapabilities::frameLatency (ADR 0057).
    void setMaxFrameLatency(uint32_t frames);
    // Waits until a new frame may start: Ok or Timeout.
    Result<void> waitForNextFrame(uint64_t timeoutNs);

    uint64_t userData() const;
    void setUserData(uint64_t value);

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

protected:
    Swapchain() = default;
    ~Swapchain() = default;
};

} // namespace slag
