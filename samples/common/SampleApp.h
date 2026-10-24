#pragma once

// Shared sample code: an SDL3 window, a Slag device and swapchain, and the
// frame loop with resize and minimize handling. Samples double as smoke
// tests (CLAUDE.md §12): --frames N renders N frames, resizes the window
// once, and exits non-zero on any error message.

#include <slag/Slag.h>

#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <vector>

struct SDL_Window;

struct SampleOptions
{
    slag::Backend backend = slag::InstanceDesc{}.backend;
    uint32_t frames = 0;     // 0: run until the window closes
    std::string videoDriver; // SDL video driver: "wayland", "x11", "windows"; empty for SDL's choice
    bool xcb = false;        // on X11, pass an XCB window instead of Xlib
};

SampleOptions parseOptions(int argc, char** argv);

// Both shader formats of one entry point, alive while pipelines are created.
struct SampleShader
{
    std::vector<char> spirv;
    std::vector<char> dxil;
    std::vector<slag::ShaderBinary> binaries;

    SampleShader(const std::string& name, const std::string& entry);
    slag::ShaderStage stage(const char* entry) const
    {
        return {.binaries = binaries, .entryPoint = entry};
    }
};

class SampleApp
{
public:
    SampleApp(const char* title, const SampleOptions& options);
    ~SampleApp();

    bool ok() const
    {
        return swapchain != nullptr;
    }

    // Records one frame into `commandBuffer`: the target is the swapchain
    // image's sRGB view, already in the RenderTarget layout; the frame loop
    // moves it to Present afterwards.
    using RecordFn = std::function<void(slag::CommandBuffer* commandBuffer, slag::AttachmentView* target,
                                        uint32_t width, uint32_t height)>;

    // Returns the process exit code.
    int run(const RecordFn& record);

    // Submits one command buffer and waits for it (for uploads).
    bool submitAndWait(slag::CommandBuffer* commandBuffer);

    // Picks the first of `preferred` the swapchain can switch to; Fifo otherwise.
    void preferPresentModes(std::span<const slag::PresentMode> preferred);

    // Inside the record callback: the value this frame's submit signals.
    slag::SemaphoreValue frameSignal() const
    {
        return {frameSemaphore, frameNumber + 1};
    }

    slag::Instance* instance = nullptr;
    slag::GraphicsDevice* device = nullptr;
    slag::CommandQueue* queue = nullptr;
    slag::Swapchain* swapchain = nullptr;
    slag::PixelFormat colorFormat = slag::PixelFormat::B8G8R8A8UnormSrgb; // of the views samples render to
    uint32_t errors = 0;

private:
    bool reconfigure();

    SampleOptions options;
    slag::PresentMode presentMode = slag::PresentMode::Fifo;
    SDL_Window* window = nullptr;
    slag::NativeWindow nativeWindow;
    uint32_t width = 0;
    uint32_t height = 0;
    static constexpr uint32_t framesInFlight = 2;
    slag::CommandBuffer* commandBuffers[framesInFlight] = {};
    slag::Semaphore* frameSemaphore = nullptr;
    uint64_t frameNumber = 0;
};
