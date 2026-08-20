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

    std::unique_ptr<SwapChain> createSwapChain(slag::GraphicsCard* card, uint32_t width, uint32_t height, PixelFormat format, PresentMode presentMode, uint32_t backBuffers);
}



#endif //SLAG_PLATFORMUTILITIES_H
