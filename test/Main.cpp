#define SDL_MAIN_HANDLED
#include <slag/SlagLib.h>
#include <SDL.h>
#include <SDL_syswm.h>
#ifdef _WIN32
#include <windows.h>
#elif __linux

#endif
int main()
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
#elif __linux
    pd.nativeWindowHandle = reinterpret_cast<void*>(wmInfo.info.x11.window);
    pd.nativeDisplayType = wmInfo.info.x11.display;
#endif

    slag::SlagLib::initialize({slag::VULKAN});

    slag::Swapchain* swapchain = slag::Swapchain::create(pd,500, 500, 2, true);

    bool quit = false;
    while(!quit)
    {
        SDL_Event e;
        while(SDL_PollEvent(&e))
        {
            if( e.type == SDL_QUIT )
            {
                quit = true;
            }
        }

        if(auto frame = swapchain->next())
        {
            frame->begin();
            auto commandBuffer = frame->getCommandBuffer();
            slag::ImageMemoryBarrier imageBarrier{.oldLayout = slag::Texture::Layout::UNDEFINED, .newLayout = slag::Texture::Layout::PRESENT, .texture=frame->getBackBuffer()};
            commandBuffer->insertImageBarrier(imageBarrier,slag::PipelineStageFlags::COLOR_ATTACHMENT_OUTPUT,slag::PipelineStageFlags::BOTTOM);
            frame->end();
        }
    }
    delete swapchain;

    slag::SlagLib::cleanup();

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}