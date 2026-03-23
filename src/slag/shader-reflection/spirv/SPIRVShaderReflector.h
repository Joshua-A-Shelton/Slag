#ifndef SLAG_SPIRVSHADERREFLECTOR_H
#define SLAG_SPIRVSHADERREFLECTOR_H
#include "../../core/ShaderModule.h"
namespace slag
{
    class SPIRVShaderReflector
    {
    public:
        static ShaderMetaData GetMetaData(void* data, uint32_t dataLength);
    };
} // slag

#endif //SLAG_SPIRVSHADERREFLECTOR_H
