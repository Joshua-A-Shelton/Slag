#include "ShaderPipeline.h"

#include <bit>
#include <bitset>
#include <cstring>
#include <fstream>

#include "slag/backends/Backend.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    ShaderCode::ShaderCode(ShaderStageFlags stage, CodeLanguage language, void* data, size_t dataLength)
    {

        SLAG_ASSERT(std::popcount((uint16_t)stage) == 1 && "Only one stage can be set per Shader Code instance");
        _stage = stage;
        _codeLanguage = language;
        _data.resize(dataLength);
        memcpy(_data.data(), data, dataLength);
    }

    ShaderCode::ShaderCode(ShaderStageFlags stage, CodeLanguage language, std::filesystem::path path)
    {
        SLAG_ASSERT(std::popcount((uint16_t)stage) == 1 && "Only one stage can be set per Shader Code instance");
        _stage = stage;
        _codeLanguage = language;
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if(!file.is_open())
        {
            throw std::runtime_error("Unable to open shader module file");
        }

        size_t size = (size_t)file.tellg();

        _data.resize(size);

        file.seekg(0);

        file.read(reinterpret_cast<std::istream::char_type*>(_data.data()), size);

        file.close();
    }

    ShaderCode::ShaderCode(ShaderCode&& from)
    {
        move(from);
    }

    ShaderCode& ShaderCode::operator=(ShaderCode&& from)
    {
        move(from);
        return *this;
    }

    void* ShaderCode::data()
    {
        return _data.data();
    }

    size_t ShaderCode::dataSize()
    {
        return _data.size();
    }

    ShaderStageFlags ShaderCode::stage()
    {
        return _stage;
    }

    ShaderCode::CodeLanguage ShaderCode::language()
    {
        return _codeLanguage;
    }

    void ShaderCode::move(ShaderCode& from)
    {
        _stage = from._stage;
        _codeLanguage = from._codeLanguage;
        std::swap(_data, from._data);
    }

    ShaderPipeline* ShaderPipeline::newShaderPipeline(ShaderCode** shaders, size_t shaderCount, ShaderProperties& properties, VertexDescription& vertexDescription,FrameBufferDescription& framebufferDescription)
    {
        return Backend::current()->newShaderPipeline(shaders, shaderCount, properties, vertexDescription, framebufferDescription);
    }

    ShaderPipeline* ShaderPipeline::newShaderPipeline(const ShaderCode& computeShader)
    {
        return Backend::current()->newShaderPipeline(computeShader);
    }
}
