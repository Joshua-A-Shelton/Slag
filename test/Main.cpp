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

    slag::SwapchainBuilder builder(pd);
    slag::Swapchain* swapchain = builder.setWidth(500).setHeight(500).setDesiredBackBuffers(2).setVSyncEnabled(true).create();
    slag::FramebufferDescription description;
    description.addColorTarget(swapchain->imageFormat());
    slag::Shader* shader = slag::Shader::create("resources/basic.vert.spv","resources/basic.frag.spv",description);


    float colors[8]{1.0f,0.0f,0.0f,1.0f, .2f,.1f,.7f,1.0f};
    float colors2[4]{0.0f,1.0f,0.0f,0.0f};
    float verts[15]{ 1.f, 1.f, 0.0f,.5,.5,   -1.f, 1.f, 0.0f,.5,.5,  0.f,-1.f, 0.0f,.5,.5};
    auto buffer = slag::Buffer::create(verts,sizeof(verts),slag::Buffer::CPU_TO_GPU);

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
            else if(e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                int w,h;
                SDL_GetWindowSize(window,&w,&h);
                swapchain->resize(w,h);
            }
        }

        if(auto frame = swapchain->next())
        {
            frame->begin();
            auto commandBuffer = frame->getCommandBuffer();
            slag::ImageMemoryBarrier imageBarrier{.oldLayout = slag::Texture::Layout::PRESENT, .newLayout = slag::Texture::Layout::RENDER_TARGET, .texture=frame->getBackBuffer()};
            commandBuffer->insertImageBarrier(imageBarrier,slag::PipelineStageFlags::COLOR_ATTACHMENT_OUTPUT,slag::PipelineStageFlags::BOTTOM);

            slag::Attachment colorAttachment{frame->getBackBuffer(), true,{0.1f,0.3f,0.7f,1.0f}};
            slag::Rectangle rect{{0,0},{swapchain->width(),swapchain->height()}};
            commandBuffer->setTargetFramebuffer(rect,&colorAttachment,1);
            commandBuffer->setViewport(rect);
            commandBuffer->setScissors(rect);

            commandBuffer->bindShader(shader);
            commandBuffer->bindUniformData(shader,shader->getUniformSet(0),0,frame->getUniformBuffer(),colors,sizeof(colors));
            commandBuffer->bindUniformData(shader,shader->getUniformSet(1),0,frame->getUniformBuffer(),colors2,sizeof(colors2));
            commandBuffer->bindVertexBuffer(buffer);
            commandBuffer->draw(3,1,0,0);

            commandBuffer->endTargetFramebuffer();

            slag::ImageMemoryBarrier imageBarrier2{.oldLayout = slag::Texture::Layout::RENDER_TARGET, .newLayout = slag::Texture::Layout::PRESENT, .texture=frame->getBackBuffer()};
            commandBuffer->insertImageBarrier(imageBarrier2,slag::PipelineStageFlags::COLOR_ATTACHMENT_OUTPUT,slag::PipelineStageFlags::BOTTOM);
            frame->end();
        }
    }
    delete buffer;
    delete shader;
    delete swapchain;

    slag::SlagLib::cleanup();

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}