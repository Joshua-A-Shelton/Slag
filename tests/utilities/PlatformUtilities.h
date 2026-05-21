#ifndef SLAG_PLATFORMUTILITIES_H
#define SLAG_PLATFORMUTILITIES_H
#include <slag/Slag.h>
#include <memory>

struct ShaderModule
{
    slag::ShaderCode details;
    std::vector<std::byte> loadedCode;
};
ShaderModule slagTestsCreateShaderModule(slag::GraphicsCard* card, const std::string& shaderIdentifier);


#endif //SLAG_PLATFORMUTILITIES_H
