#ifndef SLAG_UTILITIES_H
#define SLAG_UTILITIES_H

#include <unordered_map>
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
        std::unique_ptr<SwapChain> createSwapChain(SDL_Window* window, const slag::SwapChainDetails& details);

        bool matchesSimilarity(Buffer* compare,const std::filesystem::path& against, float overallSimilarityScore, float individualPixelScore);

        struct DescriptorEntry
        {
            std::string name;
            uint32_t index;
        };
        struct DescriptorDictionary
        {
        public:
            DescriptorDictionary(uint32_t groups);
            DescriptorEntry getEntry(uint32_t group, std::string originalName)const;
            void addEntry(uint32_t group, const std::string& originalName, uint32_t index, const std::string& name);
        private:
            std::vector<std::unordered_map<std::string,DescriptorEntry>> _entries;
        };
    }
} // slag

#endif //SLAG_UTILITIES_H
