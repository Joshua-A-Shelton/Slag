#include "gtest/gtest.h"
#include "slag/SlagLib.h"
#include <SDL.h>
#include <SDL_syswm.h>
#ifdef _WIN32
#include <windows.h>
#elif __linux

#endif

using namespace slag;

struct SDL_WindowCustomDeleter
{
    void operator()(SDL_Window* window)
    {
        SDL_DestroyWindow(window);
    }
};

TEST(Swapchain, PresentModes)
{
#ifdef __linux
#ifndef NDEBUG
    GTEST_SKIP();
#endif
#endif
    SDL_WindowFlags flags = static_cast<SDL_WindowFlags>(SDL_WINDOW_RESIZABLE);
    if(SlagLib::usingBackEnd() == BackEnd::Vulkan)
    {
        flags = static_cast<SDL_WindowFlags>(SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);
    }
    auto window = std::unique_ptr<SDL_Window,SDL_WindowCustomDeleter>(SDL_CreateWindow("Hello, Slag",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,500,500,flags));


    slag::PlatformData pd{};

    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(window.get(), &wmInfo);
#ifdef _WIN32
    pd.nativeWindowHandle = wmInfo.info.win.window;
    pd.nativeDisplayType = wmInfo.info.win.hinstance;
#elif __linux
    pd.nativeWindowHandle = reinterpret_cast<void*>(wmInfo.info.x11.window);
    pd.nativeDisplayType = wmInfo.info.x11.display;
#endif

    auto swapchain = std::unique_ptr<Swapchain>(Swapchain::newSwapchain(pd, 500, 500, 3, Swapchain::PresentMode::MAILBOX, Pixels::Format::B8G8R8A8_UNORM));
    Uint64 totalStart = SDL_GetPerformanceCounter();
    Uint64 last = totalStart;
    for(int i=0; i< 300; i++)
    {
        if(auto frame = swapchain->next())
        {
            auto cb = frame->commandBuffer();
            cb->begin();
            cb->clearColorImage(frame->backBuffer(), {.floats={i/300.0f, 0, 0, 1}}, Texture::Layout::UNDEFINED, Texture::Layout::PRESENT,PipelineStageFlags::NONE,PipelineStageFlags::ALL_GRAPHICS);
            cb->end();
            SlagLib::graphicsCard()->graphicsQueue()->submit(&cb,1, nullptr,0, nullptr,0,frame);
        }
    }
    Uint64 totalEnd = SDL_GetPerformanceCounter();
    auto discardTime = ((totalEnd-totalStart)/1000)/300;

    swapchain->presentMode(Swapchain::PresentMode::FIFO);

    totalStart = SDL_GetPerformanceCounter();
    for(int i=0; i< 300; i++)
    {
        if(auto frame = swapchain->next())
        {
            auto cb = frame->commandBuffer();
            cb->begin();
            cb->clearColorImage(frame->backBuffer(), {.floats={i/300.0f, 0, i/300.0f, 1}}, Texture::Layout::UNDEFINED, Texture::Layout::PRESENT,PipelineStageFlags::NONE,PipelineStageFlags::ALL_GRAPHICS);
            cb->end();
            SlagLib::graphicsCard()->graphicsQueue()->submit(&cb,1, nullptr,0, nullptr,0,frame);

        }
    }
    totalEnd = SDL_GetPerformanceCounter();
    auto sequentialTime = ((totalEnd-totalStart)/1000)/300;
    GTEST_ASSERT_GE(sequentialTime,(discardTime*1.05));
}

TEST(Swapchain, NextIfReady)
{
#ifdef __linux
#ifndef NDEBUG
    GTEST_SKIP();
#endif
#endif
    SDL_WindowFlags flags = static_cast<SDL_WindowFlags>(SDL_WINDOW_RESIZABLE);
    if(SlagLib::usingBackEnd() == BackEnd::Vulkan)
    {
        flags = static_cast<SDL_WindowFlags>(SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);
    }
    auto window = std::unique_ptr<SDL_Window,SDL_WindowCustomDeleter>(SDL_CreateWindow("Hello, Slag",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,500,500,flags));


    slag::PlatformData pd{};

    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(window.get(), &wmInfo);
#ifdef _WIN32
    pd.nativeWindowHandle = wmInfo.info.win.window;
    pd.nativeDisplayType = wmInfo.info.win.hinstance;
#elif __linux
    pd.nativeWindowHandle = reinterpret_cast<void*>(wmInfo.info.x11.window);
    pd.nativeDisplayType = wmInfo.info.x11.display;
#endif

    auto swapchain = std::unique_ptr<Swapchain>(Swapchain::newSwapchain(pd, 500, 500, 2, Swapchain::PresentMode::FIFO, Pixels::Format::B8G8R8A8_UNORM));
    int frameCount = 0;
    int i=0;
    for(;;)
    {
        i++;
        if(auto frame = swapchain->nextIfReady())
        {
            auto cb = frame->commandBuffer();
            cb->begin();
            cb->clearColorImage(frame->backBuffer(), {.floats={0, 1, 0, 1}}, Texture::Layout::UNDEFINED, Texture::Layout::PRESENT,PipelineStageFlags::NONE,PipelineStageFlags::ALL_GRAPHICS);
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
            cb->clearColorImage(frame->backBuffer(), {.floats={0, 0, 1, 1}}, Texture::Layout::UNDEFINED, Texture::Layout::PRESENT,PipelineStageFlags::NONE,PipelineStageFlags::ALL_GRAPHICS);
            cb->end();
            SlagLib::graphicsCard()->graphicsQueue()->submit(&cb,1, nullptr,0, nullptr,0,frame);
            frameCount2++;
        }
        if(frameCount2 >16 || j == INT_MAX)
        {
            break;
        }
    }

    GTEST_ASSERT_GE(i,frameCount);
    GTEST_ASSERT_EQ(j,frameCount2-1);
}