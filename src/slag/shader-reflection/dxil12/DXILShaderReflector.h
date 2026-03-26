#ifndef SLAG_DXILSHADERREFLECTOR_H
#define SLAG_DXILSHADERREFLECTOR_H
#include "../../core/ShaderModule.h"

namespace slag
{
    class DXILShaderReflector
    {
    public:
        static ShaderMetaData GetMetaData(void* data, uint32_t dataLength);
    };
} // slag

#endif //SLAG_DXILSHADERREFLECTOR_H
