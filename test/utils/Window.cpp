#include "Window.h"
namespace slag
{
    std::unique_ptr<SDL_Window, SDL_WindowCustomDeleter> Window::makeWindow(const std::string& name, int width, int height)
    {
        SDL_WindowFlags flags = static_cast<SDL_WindowFlags>(SDL_WINDOW_RESIZABLE);
        if(SlagLib::usingBackEnd() == BackEnd::Vulkan)
        {
            flags = static_cast<SDL_WindowFlags>(SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);
        }
        return std::unique_ptr<SDL_Window,SDL_WindowCustomDeleter>(SDL_CreateWindow(name.c_str(),SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,width,height,flags));
    }

    std::unique_ptr<Swapchain> Window::makeSwapchain(SDL_Window* window, uint8_t backBuffers, Swapchain::PresentMode presentMode, Pixels::Format format)
    {
        slag::PlatformData pd{};

        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        SDL_GetWindowWMInfo(window, &wmInfo);
#ifdef _WIN32
        pd.nativeWindowHandle = wmInfo.info.win.window;
        pd.nativeDisplayType = wmInfo.info.win.hinstance;
#elif __linux
        pd.nativeWindowHandle = reinterpret_cast<void*>(wmInfo.info.x11.window);
    pd.nativeDisplayType = wmInfo.info.x11.display;
#endif

        int w,h;
        SDL_GetWindowSize(window,&w,&h);
        return std::unique_ptr<Swapchain>(Swapchain::newSwapchain(pd, w, h, backBuffers, presentMode, format));
    }
} // slag