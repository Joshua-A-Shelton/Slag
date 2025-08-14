#include "Utilities.h"
#define STB_IMAGE_IMPLEMENTATION
#include <iostream>
#include <stb_image.h>
#include <functional>
#include <slag/backends/Backend.h>

#include "GraphicsAPIEnvironment.h"
#include "SDL_syswm.h"
template<typename T>
using unique_ptr_custom = std::unique_ptr<T,std::function<void(T*)>>;
namespace slag
{
    std::unique_ptr<Texture> utilities::loadTextureFromFile(const std::filesystem::path& path)
    {
        int width, height, channels;
        stbi_image_free(nullptr);
        unique_ptr_custom<stbi_uc> data(stbi_load(path.string().c_str(), &width, &height,&channels,4),[](stbi_uc* uc){stbi_image_free(uc);});
        return std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::SAMPLED_IMAGE,width,height,1,1,Texture::SampleCount::ONE,data.get(),1,1));
    }

    std::vector<unsigned char> utilities::loadTexelsFromFile(const std::filesystem::path& path)
    {
        int width, height, channels;
        stbi_image_free(nullptr);
        unique_ptr_custom<stbi_uc> data(stbi_load(path.string().c_str(), &width, &height,&channels,4),[](stbi_uc* uc){stbi_image_free(uc);});
        std::vector<unsigned char> result(width*height*4);
        memcpy(result.data(),data.get(),width*height*4);
        return result;
    }

    std::unique_ptr<SDL_Window, utilities::SDL_WindowCustomDeleter> utilities::createWindow(const std::string& name,int width, int height)
    {
        return std::unique_ptr<SDL_Window,SDL_WindowCustomDeleter>(SDL_CreateWindow(name.c_str(),SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,width,height,GraphicsAPIEnvironment::graphicsAPIEnvironment()->windowFlags()));
    }

    std::unique_ptr<SwapChain> utilities::createSwapChain(SDL_Window* window, uint8_t backBuffers,SwapChain::PresentMode presentMode, Pixels::Format format,FrameResources*(* createResourceFunction)(uint8_t frameIndex, SwapChain* inChain))
    {
        slag::PlatformData pd{};

        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        SDL_GetWindowWMInfo(window, &wmInfo);
#ifdef _WIN32
        pd.platform = Platform::WIN_32;
        pd.details.win32.hwnd = wmInfo.info.win.window;
        pd.details.win32.hinstance = wmInfo.info.win.hinstance;
#elif __linux
        pd.platform = Platform::X11;
        pd.details.x11.window = wmInfo.info.x11.window;
        pd.details.x11.display = wmInfo.info.x11.display;
#endif

        int w,h;
        SDL_GetWindowSize(window,&w,&h);
        return std::unique_ptr<SwapChain>(SwapChain::newSwapChain(pd, w, h, presentMode, backBuffers, format,SwapChain::AlphaCompositing::IGNORE_ALPHA,createResourceFunction));
    }
} // slag
