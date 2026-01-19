#include "ShaderPipeline.h"

#include <bit>
#include <bitset>
#include <cstring>
#include <fstream>

#include "slag/backends/Backend.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    std::vector<ShaderCode::CodeLanguage> ShaderPipeline::acceptedLanguages()
    {
        return Backend::current()->acceptedLanuages();
    }

    ShaderPipeline* ShaderPipeline::newShaderPipeline(ShaderCode** shaders, uint32_t shaderCount, ShaderProperties& properties, VertexDescription& vertexDescription, FrameBufferDescription& framebufferDescription, DescriptorIdentity
                                                      (*identify)(const DescriptorIdentityParameters&, void*), void* identifyData)
    {
        return Backend::current()->newShaderPipeline(shaders, shaderCount, properties, vertexDescription, framebufferDescription,identify,identifyData);
    }

    ShaderPipeline* ShaderPipeline::newShaderPipeline(const ShaderCode& computeShader, DescriptorIdentity (*identify)(const DescriptorIdentityParameters&, void*), void* identifyData)
    {
        return Backend::current()->newShaderPipeline(computeShader,identify,identifyData);
    }
}
