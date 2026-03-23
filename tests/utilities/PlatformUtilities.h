#ifndef SLAG_PLATFORMUTILITIES_H
#define SLAG_PLATFORMUTILITIES_H
#include <slag/Slag.h>
#include <memory>

std::unique_ptr<slag::ShaderModule> slagTestsCreateShaderModule(slag::GraphicsCard* card, const std::string& shaderIdentifier);


#endif //SLAG_PLATFORMUTILITIES_H
