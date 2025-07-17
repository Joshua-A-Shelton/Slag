#ifndef SLAG_UTILITIES_H
#define SLAG_UTILITIES_H
#include <slag/Slag.h>
namespace slag
{
    namespace utilities
    {
        std::unique_ptr<Texture> loadTextureFromFile(const std::filesystem::path& path);
    }
} // slag

#endif //SLAG_UTILITIES_H
