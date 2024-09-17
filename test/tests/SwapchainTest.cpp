#include "gtest/gtest.h"
#include "slag/SlagLib.h"
#include <SDL.h>
#include <SDL_syswm.h>
#ifdef _WIN32
#include <windows.h>
#elif __linux

#endif

using namespace slag;

TEST(Swapchain, PresentModes)
{

    SDL_WindowFlags flags = static_cast<SDL_WindowFlags>(SDL_WINDOW_RESIZABLE);
#ifndef _WIN32
    flags |= static_cast<SDL_WindowFlags>(SDL_VULKAN);
#endif
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

    auto swapchain = Swapchain::newSwapchain(pd,500,500,3,Swapchain::PresentMode::Discard,Pixels::Format::B8G8R8A8_UNORM);
    Uint64 totalStart = SDL_GetPerformanceCounter();
    for(int i=0; i< 300; i++)
    {
        if(auto frame = swapchain->next())
        {
            auto cb = frame->commandBuffer();
            cb->begin();

            ImageBarrier barrier{.texture=frame->backBuffer(),.oldLayout=Texture::Layout::UNDEFINED, .newLayout = Texture::Layout::TRANSFER_DESTINATION};
            cb->insertBarriers(&barrier,1, nullptr,0);
            cb->clearColorImage(frame->backBuffer(), {.floats={1, 0, 0, 1}}, Texture::Layout::TRANSFER_DESTINATION);
            barrier.oldLayout = Texture::Layout::TRANSFER_DESTINATION;
            barrier.newLayout = Texture::Layout::PRESENT;
            cb->insertBarriers(&barrier,1, nullptr,0);
            cb->end();
            SlagLib::graphicsCard()->graphicsQueue()->submit(&cb,1, nullptr,0, nullptr,0,frame);

        }
    }
    Uint64 totalEnd = SDL_GetPerformanceCounter();
    auto discardTime = ((totalEnd-totalStart)/1000)/300;

    swapchain->presentMode(Swapchain::PresentMode::Sequential);

    totalStart = SDL_GetPerformanceCounter();
    for(int i=0; i< 300; i++)
    {
        if(auto frame = swapchain->next())
        {
            auto cb = frame->commandBuffer();
            cb->begin();

            ImageBarrier barrier{.texture=frame->backBuffer(),.oldLayout=Texture::Layout::UNDEFINED, .newLayout = Texture::Layout::TRANSFER_DESTINATION};
            cb->insertBarriers(&barrier,1, nullptr,0);
            cb->clearColorImage(frame->backBuffer(), {.floats={1, 0, 1, 1}}, Texture::Layout::TRANSFER_DESTINATION);
            barrier.oldLayout = Texture::Layout::TRANSFER_DESTINATION;
            barrier.newLayout = Texture::Layout::PRESENT;
            cb->insertBarriers(&barrier,1, nullptr,0);
            cb->end();
            SlagLib::graphicsCard()->graphicsQueue()->submit(&cb,1, nullptr,0, nullptr,0,frame);

        }
    }
    totalEnd = SDL_GetPerformanceCounter();
    auto sequentialTime = ((totalEnd-totalStart)/1000)/300;
    GTEST_ASSERT_GE(sequentialTime,discardTime);

    delete swapchain;

    SDL_DestroyWindow(window);
}

TEST(Swapchain, NextIfReady)
{
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

    auto swapchain = Swapchain::newSwapchain(pd,500,500,2,Swapchain::PresentMode::Sequential,Pixels::Format::B8G8R8A8_UNORM);
    int frameCount = 0;
    int i=0;
    for(;; i++)
    {
        if(auto frame = swapchain->nextIfReady())
        {
            auto cb = frame->commandBuffer();
            cb->begin();


            ImageBarrier barrier{.texture=frame->backBuffer(),.oldLayout=Texture::Layout::UNDEFINED, .newLayout = Texture::Layout::TRANSFER_DESTINATION};
            cb->insertBarriers(&barrier,1, nullptr,0);
            cb->clearColorImage(frame->backBuffer(), {.floats={0, 1, 0, 1}}, Texture::Layout::TRANSFER_DESTINATION);
            barrier.oldLayout = Texture::Layout::TRANSFER_DESTINATION;
            barrier.newLayout = Texture::Layout::PRESENT;
            cb->insertBarriers(&barrier,1, nullptr,0);
            cb->end();
            SlagLib::graphicsCard()->graphicsQueue()->submit(&cb,1, nullptr,0, nullptr,0,frame);

            frameCount++;
        }
        if(frameCount > 16 || i == INT_MAX)
        {
            break;
        }
    }
    int j=0;
    int frameCount2=0;
    for(;; j++)
    {
        if(auto frame = swapchain->next())
        {
            auto cb = frame->commandBuffer();
            cb->begin();

            ImageBarrier barrier{.texture=frame->backBuffer(),.oldLayout=Texture::Layout::UNDEFINED, .newLayout = Texture::Layout::TRANSFER_DESTINATION};
            cb->insertBarriers(&barrier,1, nullptr,0);
            cb->clearColorImage(frame->backBuffer(), {.floats={0, 0, 1, 1}}, Texture::Layout::TRANSFER_DESTINATION);
            barrier.oldLayout = Texture::Layout::TRANSFER_DESTINATION;
            barrier.newLayout = Texture::Layout::PRESENT;
            cb->insertBarriers(&barrier,1, nullptr,0);

            cb->end();
            SlagLib::graphicsCard()->graphicsQueue()->submit(&cb,1, nullptr,0, nullptr,0,frame);
            frameCount2++;
        }
        if(frameCount2 >16 || j == INT_MAX)
        {
            break;
        }
    }
    delete swapchain;

    SDL_DestroyWindow(window);

    GTEST_ASSERT_GE(i,frameCount);
    GTEST_ASSERT_EQ(j,frameCount2-1);
}