#include "PlatformUtilities.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <SDL3/SDL.h>
namespace slag::utilities
{
    ShaderModule createShaderModule(slag::GraphicsCard* card, const std::string& shaderIdentifier)
    {
        auto api = slag::Slag::backend()->api();
        std::filesystem::path shaderPath = shaderIdentifier;
        auto language =  slag::ShaderLanguage::CUSTOM;
        switch (api)
        {
        case slag::BackendAPI::VULKAN:
            shaderPath += ".spv";
            language = slag::ShaderLanguage::SPIRV;
            break;
        case slag::BackendAPI::DX12:
            shaderPath += ".dxil";
            language = slag::ShaderLanguage::DXIL;
            break;
        default:
            throw std::runtime_error("Invalid backend API");
        }
        std::ifstream file(shaderPath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file");
        }

        ShaderModule shaderModule{};

        const std::size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        shaderModule.loadedCode.resize(fileSize);
        file.read(reinterpret_cast<char*>(shaderModule.loadedCode.data()), fileSize);
        file.close();

        shaderModule.details.code = shaderModule.loadedCode.data();
        shaderModule.details.codeLength = fileSize;
        shaderModule.details.language = language;
        return shaderModule;
    }

    std::unique_ptr<SwapChain> createSwapChain(slag::GraphicsCard* card, uint32_t width, uint32_t height,
        PixelFormat format, PresentMode presentMode, uint32_t backBuffers)
    {
        slag::PlatformData pd{};

        SDL_Init(SDL_INIT_VIDEO);
        float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
        SDL_WindowFlags window_flags = SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_BORDERLESS;
        if (Slag::backend()->api() == slag::BackendAPI::VULKAN)
        {
            window_flags |= SDL_WINDOW_VULKAN;
        }
        SDL_Window* window = SDL_CreateWindow("", (int)(width * main_scale), (int)(height * main_scale), window_flags);

        auto properties = SDL_GetWindowProperties(window);
#ifdef _WIN32
        pd.platform = slag::Platform::WIN_32;
        pd.details.win32.hwnd = (HWND)SDL_GetPointerProperty(properties,SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
        pd.details.win32.hinstance = (HINSTANCE)SDL_GetPointerProperty(properties,SDL_PROP_WINDOW_WIN32_INSTANCE_POINTER, nullptr);
#else
        const char* driverName = SDL_GetCurrentVideoDriver();
        if (strcmp(driverName, "wayland") == 0)
        {
            pd.platform = slag::Platform::WAYLAND;
            pd.details.wayland.surface = static_cast<wl_surface*>(SDL_GetPointerProperty(properties,SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr));
            pd.details.wayland.display = static_cast<wl_display*>(SDL_GetPointerProperty(properties,SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr));
        }
        else
        {
            pd.platform = slag::Platform::X11;
            pd.details.x11.display = static_cast<Display*>(SDL_GetPointerProperty(properties,SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr));
            pd.details.x11.window = SDL_GetNumberProperty(properties,SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
        }
#endif

        slag::SwapChainParameters swapchainParameters{};
        swapchainParameters.imageCount = backBuffers;
        swapchainParameters.presentMode = presentMode;
        swapchainParameters.imageFormat = format;
        return std::unique_ptr<SwapChain>(card->newSwapchain(pd,width,height,swapchainParameters));
    }
}
