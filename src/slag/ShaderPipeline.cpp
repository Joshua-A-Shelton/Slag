#include "ShaderPipeline.h"
#include "BackEnd/BackEndLib.h"
#include <spirv_reflect.h>
#include <fstream>
namespace slag
{
    ShaderModule::ShaderModule(ShaderStages stage, void* data, size_t size): _stage(stage)
    {
        _shaderData.resize(size);
        memcpy(_shaderData.data(),data,size);
    }

    ShaderModule::ShaderModule(ShaderStages stage, std::filesystem::path path): _stage(stage)
    {
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if(!file.is_open())
        {
            throw std::runtime_error("Unable to open shader module file");
        }

        size_t size = (size_t)file.tellg();

        _shaderData.resize(size);

        file.seekg(0);

        file.read(_shaderData.data(), size);

        file.close();
    }

    ShaderModule::ShaderModule(ShaderModule&& from): _stage(from._stage)
    {
        move(std::move(from));
    }

    ShaderModule& ShaderModule::operator=(ShaderModule&& from)
    {
        move(std::move(from));
        return *this;
    }

    void ShaderModule::move(ShaderModule&& from)
    {
        _stage = from._stage;
        _shaderData.swap(from._shaderData);
    }

    void* ShaderModule::data()
    {
        return _shaderData.data();
    }

    size_t ShaderModule::dataSize()
    {
        return _shaderData.size();
    }

    ShaderStages ShaderModule::stage()
    {
        return _stage;
    }

    ShaderPipeline* ShaderPipeline::newShaderPipeline(ShaderModule* modules, size_t moduleCount, DescriptorGroup** descriptorGroups, size_t descriptorGroupCount, ShaderProperties& properties, VertexDescription* vertexDescription, FrameBufferDescription& frameBufferDescription)
    {
        return lib::BackEndLib::get()->newShaderPipeline(modules, moduleCount, descriptorGroups, descriptorGroupCount, properties, vertexDescription, frameBufferDescription);
    }
} // slag