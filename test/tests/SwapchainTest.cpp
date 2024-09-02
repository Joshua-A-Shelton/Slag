#include "gtest/gtest.h"
#include "slag/SlagLib.h"
#include <SDL.h>
#include <SDL_syswm.h>
#ifdef _WIN32
#include <windows.h>
#elif __linux

#endif

using namespace slag;

TEST(Swapchain, Creation)
{
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_WindowFlags flags = static_cast<SDL_WindowFlags>(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    auto window = SDL_CreateWindow("Hello, Slag",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,500,500,flags);


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

    auto swapchain = Swapchain::newSwapchain(pd,500,500,2,Swapchain::PresentMode::Sequential,Pixels::Format::R8G8B8A8_UNORM_SRGB);
    Uint64 NOW = SDL_GetPerformanceCounter();
    Uint64 LAST = 0;
    double deltaTime = 0;
    Uint64 totalStart = SDL_GetPerformanceCounter();
    for(int i=0; i< 300; i++)
    {
        if(auto frame = swapchain->next())
        {
            /*LAST = NOW;
            NOW = SDL_GetPerformanceCounter();

            deltaTime = (double)((NOW - LAST)*1000 / (double)SDL_GetPerformanceFrequency() );
            total += deltaTime;*/

            size_t thing = swapchain->currentFrameIndex();
            std::cout << "frame: " << thing << std::endl;
            auto cb = frame->commandBuffer();
            cb->begin();
            cb->ClearColorImage(frame->backBuffer(),{.uints={1,0,0,1}},Texture::Layout::PRESENT);
            cb->end();
            SlagLib::graphicsCard()->graphicsQueue()->submit(cb);
            frame->present();
        }
    }
    Uint64 totalEnd = SDL_GetPerformanceCounter();
    std::cout << "total time: "<< ((totalEnd-totalStart)/1000)/300 << std::endl;
    delete swapchain;

    SDL_DestroyWindow(window);
}