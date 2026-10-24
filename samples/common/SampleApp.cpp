#include "SampleApp.h"

#include <slag/native/D3D12Agility.h>

#include <SDL3/SDL.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>

#if defined(SAMPLE_HAS_XCB)
// In its own file: Xlib's macros (Status, None) clash with Slag's names.
void* xcbConnection(void* xlibDisplay);
#endif

// The Agility SDK runtime sits in D3D12\ next to the sample (slag_copy_agility_runtime).
SLAG_D3D12_AGILITY_SDK_EXPORTS(SAMPLE_AGILITY_SDK_VERSION, ".\\D3D12\\")

namespace {

std::vector<char> readFile(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

void printMessage(slag::MessageSeverity severity, slag::MessageSource, const char* message, void* userData)
{
    if (severity == slag::MessageSeverity::Info)
    {
        return;
    }
    std::fprintf(stderr, "%s: %s\n", severity == slag::MessageSeverity::Error ? "error" : "warning", message);
    if (severity == slag::MessageSeverity::Error)
    {
        ++*static_cast<uint32_t*>(userData);
    }
}

} // namespace

SampleOptions parseOptions(int argc, char** argv)
{
    SampleOptions options;
    for (int i = 1; i < argc; ++i)
    {
        std::string argument = argv[i];
        if (argument == "--frames" && i + 1 < argc)
        {
            options.frames = static_cast<uint32_t>(std::stoul(argv[++i]));
        }
        else if (argument == "--backend" && i + 1 < argc)
        {
            std::string name = argv[++i];
            options.backend = name == "d3d12" ? slag::Backend::D3D12 : slag::Backend::Vulkan;
        }
        else if (argument == "--video" && i + 1 < argc)
        {
            options.videoDriver = argv[++i];
        }
        else if (argument == "--xcb")
        {
            options.xcb = true;
        }
    }
    return options;
}

SampleShader::SampleShader(const std::string& name, const std::string& entry)
{
    spirv = readFile(std::string(SAMPLE_SHADER_DIR "/") + name + "." + entry + ".spv");
    dxil = readFile(std::string(SAMPLE_SHADER_DIR "/") + name + "." + entry + ".dxil");
    if (!dxil.empty())
    {
        binaries.push_back({slag::ShaderFormat::Dxil, dxil.data(), dxil.size()});
    }
    if (!spirv.empty())
    {
        binaries.push_back({slag::ShaderFormat::Spirv, spirv.data(), spirv.size()});
    }
}

SampleApp::SampleApp(const char* title, const SampleOptions& options)
    : options(options)
{
    if (!options.videoDriver.empty())
    {
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, options.videoDriver.c_str());
    }
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return;
    }
    window = SDL_CreateWindow(title, 960, 540, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window)
    {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return;
    }

    // The native window for Slag (ADR 0055).
    SDL_PropertiesID properties = SDL_GetWindowProperties(window);
    std::string driver = SDL_GetCurrentVideoDriver();
    if (driver == "windows")
    {
        nativeWindow.kind = slag::WindowKind::Win32;
        nativeWindow.display = SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WIN32_INSTANCE_POINTER, nullptr);
        nativeWindow.window =
            reinterpret_cast<uint64_t>(SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
    }
    else if (driver == "wayland")
    {
        nativeWindow.kind = slag::WindowKind::Wayland;
        nativeWindow.display = SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
        nativeWindow.window = reinterpret_cast<uint64_t>(
            SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr));
    }
    else if (driver == "x11")
    {
        nativeWindow.kind = slag::WindowKind::Xlib;
        nativeWindow.display = SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
        nativeWindow.window =
            static_cast<uint64_t>(SDL_GetNumberProperty(properties, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0));
#if defined(SAMPLE_HAS_XCB)
        if (options.xcb)
        {
            nativeWindow.kind = slag::WindowKind::Xcb;
            nativeWindow.display = xcbConnection(nativeWindow.display);
        }
#endif
    }
    std::printf("window: SDL %s driver%s\n", driver.c_str(),
                nativeWindow.kind == slag::WindowKind::Xcb ? " (XCB)" : "");

    slag::InstanceDesc instanceDesc;
    instanceDesc.backend = options.backend;
    instanceDesc.validationFlags = slag::ValidationFlags::Api;
    instanceDesc.validationFailureAction = slag::ValidationFailureAction::Continue;
    instanceDesc.messageCallback = printMessage;
    instanceDesc.messageUserData = &errors;
    instanceDesc.allowSoftwareDevices = true; // WARP in CI
    instanceDesc.applicationName = title;
    slag::Result<slag::Instance*> createdInstance = slag::createInstance(instanceDesc);
    if (!createdInstance || createdInstance.value()->graphicsDevices().empty())
    {
        std::fprintf(stderr, "no Slag device for this backend\n");
        return;
    }
    instance = createdInstance.value();
    device = instance->graphicsDevices()[0];
    std::printf("device: %s (%s)\n", device->info().name, options.backend == slag::Backend::D3D12 ? "D3D12" : "Vulkan");
    if (!device->initialize({}))
    {
        device = nullptr;
        return;
    }
    queue = device->commandQueue(slag::CommandQueueType::Graphics);
    for (slag::CommandBuffer*& commandBuffer : commandBuffers)
    {
        commandBuffer = device->createCommandBuffer({.queueType = slag::CommandQueueType::Graphics}).value();
    }
    frameSemaphore = device->createSemaphore({.debugName = "frames"}).value();

    int pixelWidth = 0;
    int pixelHeight = 0;
    SDL_GetWindowSizeInPixels(window, &pixelWidth, &pixelHeight);
    width = static_cast<uint32_t>(pixelWidth);
    height = static_cast<uint32_t>(pixelHeight);
    slag::Result<slag::Swapchain*> createdSwapchain = device->createSwapchain({.window = nativeWindow,
                                                                               .queue = queue,
                                                                               .width = width,
                                                                               .height = height,
                                                                               .imageCount = 3,
                                                                               .debugName = "sample swapchain"});
    if (createdSwapchain)
    {
        swapchain = createdSwapchain.value();
        std::printf("swapchain: %ux%u, %u images\n", width, height, swapchain->imageCount());
    }
}

SampleApp::~SampleApp()
{
    if (device)
    {
        queue->waitIdle();
        device->destroy(swapchain);
        for (slag::CommandBuffer* commandBuffer : commandBuffers)
        {
            device->destroy(commandBuffer);
        }
        device->destroy(frameSemaphore);
        device->shutdown();
    }
    slag::destroy(instance);
    if (window)
    {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
}

bool SampleApp::submitAndWait(slag::CommandBuffer* commandBuffer)
{
    slag::Semaphore* semaphore = device->createSemaphore({}).value();
    slag::SemaphoreValue done = {semaphore, 1};
    slag::CommandBuffer* list[] = {commandBuffer};
    bool ok = queue->submit({.commandBuffers = list, .signals = {&done, 1}}) &&
              device->waitSemaphores({&done, 1}, slag::WaitMode::All, UINT64_MAX);
    device->destroy(semaphore);
    return ok;
}

// The GPU must be done with every image first (ADR 0058).
bool SampleApp::reconfigure()
{
    int pixelWidth = 0;
    int pixelHeight = 0;
    SDL_GetWindowSizeInPixels(window, &pixelWidth, &pixelHeight);
    if (pixelWidth <= 0 || pixelHeight <= 0)
    {
        return false; // minimized: try again once it has a size
    }
    queue->waitIdle();
    width = static_cast<uint32_t>(pixelWidth);
    height = static_cast<uint32_t>(pixelHeight);
    bool ok = static_cast<bool>(swapchain->reconfigure({.width = width, .height = height, .imageCount = 3}));
    std::printf("reconfigured: %ux%u (%s)\n", width, height, ok ? "ok" : "failed");
    return ok;
}

int SampleApp::run(const RecordFn& record)
{
    if (!ok())
    {
        return 1;
    }
    bool running = true;
    bool minimized = false;
    bool needsReconfigure = false;
    uint32_t presented = 0;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                running = false;
                break;
            case SDL_EVENT_WINDOW_MINIMIZED:
                minimized = true;
                break;
            case SDL_EVENT_WINDOW_RESTORED:
                minimized = false;
                needsReconfigure = true;
                break;
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                // Wayland reports size only through events (ADR 0058).
                needsReconfigure = true;
                break;
            default:
                break;
            }
        }
        if (!running)
        {
            break;
        }
        if (minimized)
        {
            SDL_Delay(16);
            continue;
        }
        if (needsReconfigure)
        {
            needsReconfigure = !reconfigure();
            if (needsReconfigure)
            {
                SDL_Delay(16);
                continue;
            }
        }

        // Wait for the frame that last used this command buffer.
        uint32_t slot = static_cast<uint32_t>(frameNumber % framesInFlight);
        if (frameNumber >= framesInFlight)
        {
            slag::SemaphoreValue previous = {frameSemaphore, frameNumber - framesInFlight + 1};
            device->waitSemaphores({&previous, 1}, slag::WaitMode::All, UINT64_MAX);
        }
        slag::Result<uint32_t> acquired = swapchain->acquire();
        if (acquired.status() == slag::Status::OutOfDate)
        {
            needsReconfigure = true;
            continue;
        }
        if (!acquired)
        {
            std::fprintf(stderr, "acquire failed\n");
            return 1;
        }
        uint32_t image = acquired.value();

        slag::CommandBuffer* commandBuffer = commandBuffers[slot];
        commandBuffer->reset();
        commandBuffer->begin();
        slag::TextureBarrier toTarget = {.syncBefore = slag::BarrierSync::None,
                                         .syncAfter = slag::BarrierSync::RenderTarget,
                                         .accessAfter = slag::BarrierAccess::RenderTarget,
                                         .layoutBefore = slag::TextureLayout::Undefined,
                                         .layoutAfter = slag::TextureLayout::RenderTarget,
                                         .texture = swapchain->texture(image)};
        commandBuffer->barrier({}, {}, {&toTarget, 1});
        record(commandBuffer, swapchain->attachmentView(image, true), width, height);
        slag::TextureBarrier toPresent = {.syncBefore = slag::BarrierSync::RenderTarget,
                                          .syncAfter = slag::BarrierSync::None,
                                          .accessBefore = slag::BarrierAccess::RenderTarget,
                                          .accessAfter = slag::BarrierAccess::NoAccess,
                                          .layoutBefore = slag::TextureLayout::RenderTarget,
                                          .layoutAfter = slag::TextureLayout::Present,
                                          .texture = swapchain->texture(image)};
        commandBuffer->barrier({}, {}, {&toPresent, 1});
        commandBuffer->end();

        ++frameNumber;
        slag::SemaphoreValue signal = {frameSemaphore, frameNumber};
        slag::Swapchain* acquiredSwapchains[] = {swapchain};
        slag::CommandBuffer* list[] = {commandBuffer};
        queue->submit({.commandBuffers = list, .signals = {&signal, 1}, .acquiredSwapchains = acquiredSwapchains});
        slag::Result<void> result = queue->present(swapchain, slag::PresentMode::Fifo);
        if (result.status() == slag::Status::Suboptimal || result.status() == slag::Status::OutOfDate)
        {
            needsReconfigure = true;
        }
        else if (result.status() == slag::Status::Occluded)
        {
            SDL_Delay(16);
        }
        else if (!result)
        {
            std::fprintf(stderr, "present failed\n");
            return 1;
        }
        ++presented;

        // Smoke mode: resize once halfway, stop after the requested frames.
        if (options.frames != 0)
        {
            if (presented == options.frames / 2)
            {
                SDL_SetWindowSize(window, 640, 480);
            }
            if (presented >= options.frames)
            {
                running = false;
            }
        }
    }
    queue->waitIdle();
    std::printf("%u frames, %u errors\n", presented, errors);
    return errors == 0 ? 0 : 1;
}
