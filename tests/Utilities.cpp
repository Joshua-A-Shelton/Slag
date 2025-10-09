#include "Utilities.h"
#define STB_IMAGE_IMPLEMENTATION
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
        if (!std::filesystem::exists(path))
        {
            throw std::runtime_error("Texture does not exist");
        }
        int width, height, channels;
        unique_ptr_custom<stbi_uc> data(stbi_load(path.string().c_str(), &width, &height,&channels,4),[](stbi_uc* uc){stbi_image_free(uc);});
        TextureBufferMapping mapping
        {
            .bufferOffset = 0,
            .textureSubresource =
         {
                .aspectFlags = Pixels::AspectFlags::COLOR,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .textureOffset = {0,0,0},
            .textureExtent = {(uint32_t)width,(uint32_t)height,1}
        };
        return std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::SAMPLED_IMAGE,width,height,1,1,1,Texture::SampleCount::ONE,data.get(),width*height*Pixels::size(Pixels::Format::R8G8B8A8_UNORM),&mapping,1));
    }

    std::vector<unsigned char> utilities::loadTexelsFromFile(const std::filesystem::path& path)
    {
        if (!std::filesystem::exists(path))
        {
            throw std::runtime_error("Texture does not exist");
        }
        int width, height, channels;
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

    bool utilities::matchesSimilarity(Buffer* compare, const std::filesystem::path& against,float overallSimilarityScore, float individualPixelScore)
    {
        auto pixels = compare->as<uint8_t>();
        if (!std::filesystem::exists(against))
        {
            return false;
        }

        auto groundTruth = utilities::loadTexelsFromFile(against);

        if(groundTruth.size()!=compare->countAsArray<uint8_t>())
        {
            return false;
        }
        float maxDifference = 255.0f*(1-individualPixelScore);
        std::vector<float> pixelSimilarity(groundTruth.size()/4);
        if (maxDifference > 0)
        {
            for (auto i=0; i< groundTruth.size(); i+=4)
            {
                float drawnRed = pixels[i];
                float drawnGreen = pixels[i+1];
                float drawnBlue = pixels[i+2];
                float drawnAlpha = pixels[i+3];

                float groundRed = groundTruth[i];
                float groundGreen = groundTruth[i+1];
                float groundBlue = groundTruth[i+2];
                float groundAlpha = groundTruth[i+3];

                float difRed = std::abs(drawnRed-groundRed);
                float difGreen = std::abs(drawnGreen-groundGreen);
                float difBlue = std::abs(drawnBlue-groundBlue);
                float difAlpha = std::abs(drawnAlpha-groundAlpha);

                float pixelDifference = (difRed + difGreen + difBlue + difAlpha)/4;

                if (individualPixelScore > 0)
                {
                    if (pixelDifference> maxDifference)
                    {
                        return false;
                    }
                }
                float percentSimilar = (255.0f-pixelDifference)/255.0f;
                if (percentSimilar < individualPixelScore)
                {
                    return false;
                }
                pixelSimilarity[i/4] = percentSimilar;
            }
            float total = 0;
            for (int i=0; i< pixelSimilarity.size(); i++)
            {
                total += pixelSimilarity[i];
            }
            float overallSimilarity = total/pixelSimilarity.size();
            if (overallSimilarity < overallSimilarityScore)
            {
                return false;
            }
        }
        else
        {
            for (int i=0; i<groundTruth.size(); i++)
            {
                if(groundTruth[i]!=pixels[i])
                {
                    return false;
                }
            }
        }
        return true;
    }
} // slag
