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

    auto swapchain = Swapchain::newSwapchain(pd,500,500,3,Swapchain::PresentMode::Discard,Pixels::Format::B8G8R8A8_UNORM_SRGB);
    std::cout.flush();
    Uint64 totalStart = SDL_GetPerformanceCounter();
    for(int i=0; i< 300; i++)
    {
        if(auto frame = swapchain->next())
        {
            frame->begin();

            size_t thing = swapchain->currentFrameIndex();
            auto cb = frame->commandBuffer();
            ImageBarrier barrier{.texture=frame->backBuffer(),.oldLayout=Texture::Layout::UNDEFINED, .newLayout = Texture::Layout::TRANSFER_DESTINATION};
            cb->insertBarriers(&barrier,1, nullptr,0);
            cb->clearColorImage(frame->backBuffer(), {.uints={255, 0, 0, 255}}, Texture::Layout::TRANSFER_DESTINATION);
            barrier.oldLayout = Texture::Layout::TRANSFER_DESTINATION;
            barrier.newLayout = Texture::Layout::PRESENT;
            cb->insertBarriers(&barrier,1, nullptr,0);
            frame->end();
        }
    }
    Uint64 totalEnd = SDL_GetPerformanceCounter();
    std::cout << "total time: "<< ((totalEnd-totalStart)/1000)/300 << std::endl;
    delete swapchain;

    SDL_DestroyWindow(window);
}