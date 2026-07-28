#ifndef SLAG_PLATFORMUTILITIES_H
#define SLAG_PLATFORMUTILITIES_H
#include <slag/Slag.h>
#include <memory>

namespace slag::utilities
{
    struct ShaderModule
    {
        slag::ShaderCode details;
        std::vector<std::byte> loadedCode;
    };
    ShaderModule createShaderModule(slag::GraphicsCard* card, const std::string& shaderIdentifier);
}



#endif //SLAG_PLATFORMUTILITIES_H
