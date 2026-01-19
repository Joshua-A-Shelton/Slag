#ifndef SLAG_SPIRVREFLECTION_H
#define SLAG_SPIRVREFLECTION_H
#include <unordered_map>
#include <slag/Slag.h>
namespace slag
{
    namespace spirv
    {
        ShaderMetaData reflectShaderCode(ShaderCode& shaderCode);
    } // spirv
} // slag

#endif //SLAG_SPIRVREFLECTION_H
