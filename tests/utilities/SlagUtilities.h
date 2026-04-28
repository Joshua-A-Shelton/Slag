#ifndef SLAG_SLAGUTILITIES_H
#define SLAG_SLAGUTILITIES_H
#include <filesystem>
#include <memory>
#include <slag/Slag.h>
namespace slag
{
    namespace utilities
    {
        std::unique_ptr<Texture> loadTexture(std::filesystem::path const& path, GraphicsCard* graphicsCard);

        void saveTexture(std::filesystem::path const& path, Texture* texture);
    }
} // slag

#endif //SLAG_SLAGUTILITIES_H