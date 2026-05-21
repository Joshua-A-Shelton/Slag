#ifndef SLAG_SHADERCODE_H
#define SLAG_SHADERCODE_H
#include <cstdint>
#include <vector>

#include "Descriptors.h"

namespace slag
{
    enum class ShaderLanguage
    {
        SPIRV,
        DXIL,
        CUSTOM
    };

    ///Struct that contains a pointer to the shader code with some metadata
    struct ShaderCode
    {
        ShaderLanguage language;
        void* code;
        uint32_t codeLength;
    };
} // slag

#endif //SLAG_SHADERCODE_H
