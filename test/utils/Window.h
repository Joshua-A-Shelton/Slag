#ifndef SLAG_WINDOW_H
#define SLAG_WINDOW_H

#include <SDL.h>
#include <SDL_syswm.h>
#include <memory>
#include "slag/SlagLib.h"

namespace slag
{

    struct SDL_WindowCustomDeleter
    {
        void operator()(SDL_Window* window)
        {
            SDL_DestroyWindow(window);
        }
    };

    class Window
    {
    public:
        static std::unique_ptr<SDL_Window,SDL_WindowCustomDeleter> makeWindow(const std::string& name, int width, int height);
        static std::unique_ptr<Swapchain> makeSwapchain(SDL_Window* window, uint8_t backBuffers, Swapchain::PresentMode presentMode, Pixels::Format format,FrameResources* (*createResourceFunction)(size_t frameIndex, Swapchain* inChain));
    };

} // slag

#endif //SLAG_WINDOW_H
