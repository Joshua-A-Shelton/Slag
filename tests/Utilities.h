#ifndef SLAG_UTILITIES_H
#define SLAG_UTILITIES_H

#include <slag/Slag.h>
#include <SDL2/SDL.h>
namespace slag
{
    namespace utilities
    {
        std::unique_ptr<Texture> loadTextureFromFile(const std::filesystem::path& path);
        std::vector<unsigned char> loadTexelsFromFile(const std::filesystem::path& path);



        struct SDL_WindowCustomDeleter
        {
            void operator()(SDL_Window* window)
            {
                SDL_DestroyWindow(window);
            }
        };

        std::unique_ptr<SDL_Window, SDL_WindowCustomDeleter> createWindow(const std::string& name, int width, int height);
        std::unique_ptr<SwapChain> createSwapChain(SDL_Window* window, uint8_t backBuffers, SwapChain::PresentMode presentMode, Pixels::Format format,FrameResources* (*createResourceFunction)(uint8_t frameIndex, SwapChain* inChain));

        bool matchesSimilarity(Buffer* compare,const std::filesystem::path& against, float overallSimilarityScore, float individualPixelScore);
    }
} // slag

#endif //SLAG_UTILITIES_H
