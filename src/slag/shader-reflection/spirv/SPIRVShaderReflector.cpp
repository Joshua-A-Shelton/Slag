#include "SPIRVShaderReflector.h"
#include <spirv_reflect.h>

#include "slag/exceptions/InvalidShaderCodeError.h"
#include "slag/exceptions/NotImplemented.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    TextureType textureTypeFromSPV(SpvDim dim)
    {
        switch (dim)
        {
        case SpvDim1D:
            return TextureType::ONE_DIMENSIONAL;
        case SpvDim2D:
            return TextureType::TWO_DIMENSIONAL;
        case SpvDim3D:
            return TextureType::THREE_DIMENSIONAL;
        case SpvDimCube:
            return TextureType::CUBE_MAP;
        }
        throw InvalidShaderCodeError("Invalid spirv dimension");
    }

    PixelFormat pixelFormatFromSPV(SpvImageFormat format)
    {
        switch (format)
        {
        case SpvImageFormat::SpvImageFormatR8:
            return PixelFormat::R8_UNORM;
            break;
        case SpvImageFormat::SpvImageFormatR8Snorm:
            return PixelFormat::R8_SNORM;
            break;
        case SpvImageFormat::SpvImageFormatR11fG11fB10f:
            return PixelFormat::R11G11B10_FLOAT;
            break;
        case SpvImageFormat::SpvImageFormatR8ui:
            return PixelFormat::R8_UINT;
            break;
        case SpvImageFormat::SpvImageFormatR8i:
            return PixelFormat::R8_SINT;
            break;
        case SpvImageFormat::SpvImageFormatR16:
            return PixelFormat::R16_UNORM;
            break;
        case SpvImageFormat::SpvImageFormatR16f:
            return PixelFormat::R16_FLOAT;
            break;
        case SpvImageFormat::SpvImageFormatR16i:
            return PixelFormat::R16_SINT;
            break;
        case SpvImageFormat::SpvImageFormatR16Snorm:
            return PixelFormat::R16_SNORM;
            break;
        case SpvImageFormat::SpvImageFormatR16ui:
            return PixelFormat::R16_UINT;
            break;
        case SpvImageFormat::SpvImageFormatR32f:
            return PixelFormat::R32_FLOAT;
            break;
        case SpvImageFormat::SpvImageFormatR32i:
            return PixelFormat::R32_SINT;
            break;
        case SpvImageFormat::SpvImageFormatR32ui:
            return PixelFormat::R32_UINT;
            break;
        case SpvImageFormat::SpvImageFormatR64i:
            return PixelFormat::UNDEFINED;
            break;
        case SpvImageFormat::SpvImageFormatR64ui:
            return PixelFormat::UNDEFINED;
            break;
        case SpvImageFormat::SpvImageFormatRg8:
            return PixelFormat::R8G8_UNORM;
            break;
        case SpvImageFormat::SpvImageFormatRg8i:
            return PixelFormat::R8G8_SINT;
            break;
        case SpvImageFormat::SpvImageFormatRg8Snorm:
            return PixelFormat::R8G8_SNORM;
            break;
        case SpvImageFormat::SpvImageFormatRg8ui:
            return PixelFormat::R8G8_UINT;
            break;
        case SpvImageFormat::SpvImageFormatRg16:
            return PixelFormat::R16G16_UNORM;
            break;
        case SpvImageFormat::SpvImageFormatRg16f:
            return PixelFormat::R16G16_FLOAT;
            break;
        case SpvImageFormat::SpvImageFormatRg16i:
            return PixelFormat::R16G16_SINT;
            break;
        case SpvImageFormat::SpvImageFormatRg16Snorm:
            return PixelFormat::R16G16_SNORM;
            break;
        case SpvImageFormat::SpvImageFormatRg16ui:
            return PixelFormat::R16G16_UINT;
            break;
        case SpvImageFormat::SpvImageFormatRg32f:
            return PixelFormat::R32G32_FLOAT;
            break;
        case SpvImageFormat::SpvImageFormatRg32i:
            return PixelFormat::R32G32_SINT;
            break;
        case SpvImageFormat::SpvImageFormatRg32ui:
            return PixelFormat::R32G32_UINT;
            break;
        case SpvImageFormat::SpvImageFormatRgb10A2:
            return PixelFormat::R10G10B10A2_UNORM;
            break;
        case SpvImageFormat::SpvImageFormatRgb10a2ui:
            return PixelFormat::R10G10B10A2_UINT;
            break;
        case SpvImageFormat::SpvImageFormatRgba8:
            return PixelFormat::R8G8B8A8_UNORM;
            break;
        case SpvImageFormat::SpvImageFormatRgba8i:
            return PixelFormat::R8G8B8A8_SINT;
            break;
        case SpvImageFormat::SpvImageFormatRgba8Snorm:
            return PixelFormat::R8G8B8A8_SNORM;
            break;
        case SpvImageFormat::SpvImageFormatRgba8ui:
            return PixelFormat::R8G8B8A8_UINT;
            break;
        case SpvImageFormat::SpvImageFormatRgba16:
            return PixelFormat::R16G16B16A16_UNORM;
            break;
        case SpvImageFormat::SpvImageFormatRgba16f:
            return PixelFormat::R16G16B16A16_FLOAT;
            break;
        case SpvImageFormat::SpvImageFormatRgba16i:
            return PixelFormat::R16G16B16A16_SINT;
            break;
        case SpvImageFormat::SpvImageFormatRgba16Snorm:
            return PixelFormat::R16G16B16A16_SNORM;
            break;
        case SpvImageFormat::SpvImageFormatRgba16ui:
            return PixelFormat::R16G16B16A16_UINT;
            break;
        case SpvImageFormat::SpvImageFormatRgba32f:
            return PixelFormat::R32G32B32A32_FLOAT;
            break;
        case SpvImageFormat::SpvImageFormatRgba32i:
            return PixelFormat::R32G32B32A32_SINT;
            break;
        case SpvImageFormat::SpvImageFormatRgba32ui:
            return PixelFormat::R32G32B32A32_UINT;
            break;
        }
        throw InvalidShaderCodeError("Encountered unsupported pixel format in shader");
    }
    
    DescriptorType descriptorTypeFromSPV(SpvReflectDescriptorType type)
    {
        switch (type)
        {
        case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
            return DescriptorType::ACCELERATION_STRUCTURE;
        case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            throw InvalidShaderCodeError("Combined Texture/Sampler descriptors are not supported");
        case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            throw InvalidShaderCodeError("Input Attachment descriptors are not supported");
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            return DescriptorType::SAMPLED_TEXTURE;
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
            return DescriptorType::SAMPLER;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            return DescriptorType::UNORDERED_ACCESS_BUFFER;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            return DescriptorType::UNORDERED_ACCESS_BUFFER;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            return DescriptorType::UNORDERED_ACCESS_TEXTURE;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            return DescriptorType::UNORDERED_ACCESS_TEXEL_BUFFER;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            return DescriptorType::UNIFORM_BUFFER;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            return DescriptorType::UNIFORM_BUFFER;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            return DescriptorType::UNIFORM_TEXEL_BUFFER;
        }
        throw InvalidShaderCodeError("Invalid descriptor type");
    }

    ShaderType getShaderStage(SpvReflectShaderModule& module)
    {
        auto stage = ShaderType::COMPUTE;
        if (module.shader_stage & SPV_REFLECT_SHADER_STAGE_VERTEX_BIT)
        {
            stage = ShaderType::VERTEX;
        }
        else if (module.shader_stage & SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT)
        {
            stage = ShaderType::GEOMETRY;
        }
        else if (module.shader_stage & SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT)
        {
            stage = ShaderType::FRAGMENT;
        }
        else if (module.shader_stage & SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT)
        {
            stage = ShaderType::COMPUTE;
        }
        else if (module.shader_stage & SPV_REFLECT_SHADER_STAGE_RAYGEN_BIT_KHR)
        {
            stage = ShaderType::RAY_GENERATION;
        }
        else if (module.shader_stage & SPV_REFLECT_SHADER_STAGE_ANY_HIT_BIT_KHR)
        {
            stage = ShaderType::ANY_HIT;
        }
        else if (module.shader_stage & SPV_REFLECT_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
        {
            stage = ShaderType::CLOSEST_HIT;
        }
        else if (module.shader_stage & SPV_REFLECT_SHADER_STAGE_MISS_BIT_KHR)
        {
            stage = ShaderType::MISS;
        }
        else if (module.shader_stage & SPV_REFLECT_SHADER_STAGE_INTERSECTION_BIT_KHR)
        {
            stage = ShaderType::INTERSECTION;
        }
        else if (module.shader_stage & SPV_REFLECT_SHADER_STAGE_CALLABLE_BIT_KHR)
        {
            stage = ShaderType::CALLABLE;
        }
        else if (module.shader_stage & SPV_REFLECT_SHADER_STAGE_MESH_BIT_EXT)
        {
            stage = ShaderType::MESH;
        }
        else if (module.shader_stage & SPV_REFLECT_SHADER_STAGE_TASK_BIT_EXT)
        {
            stage = ShaderType::TASK;
        }
        else
        {
            throw InvalidShaderCodeError("Unsupported shader stage");
        }
        return stage;
    }

    GraphicsType getGraphicsTypeFromSPV(SpvReflectTypeDescription* typeDescription)
    {
        if (typeDescription->type_flags & SPV_REFLECT_TYPE_FLAG_STRUCT)
        {
            return GraphicsType::STRUCT;
        }

        auto vectorComponents = typeDescription->traits.numeric.vector.component_count;
        auto matrixColumns = typeDescription->traits.numeric.matrix.column_count;
        auto matrixRows = typeDescription->traits.numeric.matrix.row_count;
        auto numericSignedness = typeDescription->traits.numeric.scalar.signedness;
        auto numericSize = typeDescription->traits.numeric.scalar.width;

        bool boolType = typeDescription->type_flags & SPV_REFLECT_TYPE_FLAG_BOOL;
        bool intType = typeDescription->type_flags & SPV_REFLECT_TYPE_FLAG_INT;
        bool floatType = typeDescription->type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT;

        GraphicsTypeBits returnType = GraphicsTypeBits::NONE;
        if (boolType)
        {
            returnType |= GraphicsTypeBits::BOOLEAN_BIT;
        }
        else if (intType)
        {
            switch (numericSize)
            {
            case 8:
                returnType |= GraphicsTypeBits::INTEGER_8_BIT;
                break;
            case 16:
                returnType |= GraphicsTypeBits::INTEGER_16_BIT;
                break;
            case 32:
                returnType |= GraphicsTypeBits::INTEGER_32_BIT;
                break;
            case 64:
                returnType |= GraphicsTypeBits::INTEGER_64_BIT;
                break;
            default:
                throw InvalidShaderCodeError("Unsupported integer size");
            }
            if (!numericSignedness)
            {
                returnType |= GraphicsTypeBits::UNSIGNED_BIT;
            }
        }
        else if (floatType)
        {
            switch (numericSize)
            {
            case 16:
                returnType |= GraphicsTypeBits::FLOAT_16_BIT;
                break;
            case 32:
                returnType |= GraphicsTypeBits::FLOAT_32_BIT;
                break;
            case 64:
                returnType |= GraphicsTypeBits::FLOAT_64_BIT;
                break;
            default:
                throw InvalidShaderCodeError("Unsupported float size");
            }
        }

        if (matrixColumns)
        {
            switch (matrixColumns)
            {
            case 2:
                returnType|=GraphicsTypeBits::MATRIX2X_BIT;
                break;
            case 3:
                returnType|=GraphicsTypeBits::MATRIX3X_BIT;
                break;
            case 4:
                returnType|=GraphicsTypeBits::MATRIX4X_BIT;
                break;
            }
            switch (matrixRows)
            {
            case 2:
                returnType|=GraphicsTypeBits::VECTOR2_BIT;
                break;
            case 3:
                returnType|=GraphicsTypeBits::VECTOR3_BIT;
                break;
            case 4:
                returnType|=GraphicsTypeBits::VECTOR4_BIT;
                break;
            }
        }
        else if (vectorComponents)
        {
            switch (vectorComponents)
            {
            case 2:
                returnType|=GraphicsTypeBits::VECTOR2_BIT;
                break;
            case 3:
                returnType|=GraphicsTypeBits::VECTOR3_BIT;
                break;
            case 4:
                returnType|=GraphicsTypeBits::VECTOR4_BIT;
                break;
            }
        }
        return static_cast<GraphicsType>(returnType);

    }

    StructMember getStructMember(const SpvReflectBlockVariable& data)
    {
        std::vector<StructMember> members;
        members.reserve(data.member_count);
        for (uint32_t i = 0; i < data.member_count; i++)
        {
            members.push_back(getStructMember(data.members[i]));
        }
        GraphicsType gtype = getGraphicsTypeFromSPV(data.type_description);
        auto dims = data.array.dims[0];
        auto arrayDepth = dims == 0? 1 : dims;
        if (gtype != GraphicsType::STRUCT)
        {
            return StructMember(data.name,gtype,arrayDepth,data.offset,data.absolute_offset);
        }
        return StructMember(data.name,std::move(members),arrayDepth,data.offset,data.absolute_offset);
    }

    BufferLayout getBufferLayout(const SpvReflectBlockVariable& data)
    {
        std::vector<StructMember> structMembers;
        structMembers.reserve(data.member_count);
        for (uint32_t i = 0; i < data.member_count; i++)
        {
            structMembers.push_back(getStructMember(data.members[i]));
        }
        return BufferLayout(std::move(structMembers));
    }

    std::vector<BindGroup> getBindings(SpvReflectShaderModule& module)
    {
        std::vector<BindGroup> groups;
        groups.reserve(module.descriptor_set_count);
        for (uint32_t i = 0; i < module.descriptor_set_count; ++i)
        {
            auto set = module.descriptor_sets[i];
            std::vector<DescriptorMeta> bindings;
            bindings.reserve(set.binding_count);
            for (uint32_t j = 0; j < set.binding_count; ++j)
            {
                auto& binding = *set.bindings[j];

                auto type = descriptorTypeFromSPV(binding.descriptor_type);
                switch (type)
                {
                case DescriptorType::UNKNOWN:
                    throw InvalidShaderCodeError("Invalid descriptor type");
                    break;
                case DescriptorType::SAMPLER:
                    bindings.emplace_back(binding.name,binding.binding,type,SamplerDescription{});
                    break;
                case DescriptorType::SAMPLED_TEXTURE:
                case DescriptorType::UNORDERED_ACCESS_TEXTURE:
                    bindings.emplace_back(binding.name,binding.binding,type,TextureDescription(textureTypeFromSPV(binding.image.dim),binding.image.ms,binding.image.arrayed));
                    break;
                case DescriptorType::UNIFORM_BUFFER:
                case DescriptorType::UNORDERED_ACCESS_BUFFER:
                    bindings.emplace_back(binding.name,binding.binding,type,getBufferLayout(binding.block));
                    break;
                case DescriptorType::UNIFORM_TEXEL_BUFFER:
                case DescriptorType::UNORDERED_ACCESS_TEXEL_BUFFER:
                    bindings.emplace_back(binding.name,binding.binding,type,pixelFormatFromSPV(binding.image.image_format));
                    break;
                case DescriptorType::ACCELERATION_STRUCTURE:
                    throw NotImplemented();
                    break;
                }
            }
            groups.emplace_back(set.set,std::move(bindings));
        }
        return groups;
    }

    std::vector<InputVariable> getInputVariables(SpvReflectShaderModule& module)
    {
        std::vector<InputVariable> inputVariables;
        inputVariables.reserve(module.input_variable_count);

        for (uint32_t i = 0; i < module.input_variable_count; i++)
        {
            auto inputVariable = module.input_variables[i];
            inputVariables.emplace_back(inputVariable->name,getGraphicsTypeFromSPV(inputVariable->type_description),inputVariable->location);
        }

        return inputVariables;
    }

    ShaderMetaData SPIRVShaderReflector::GetMetaData(void* data, uint32_t dataLength)
    {
        SpvReflectShaderModule shaderModule;
        ShaderType shaderStage = ShaderType::VERTEX;
        std::vector<BindGroup> bindGroups;
        std::vector<InputVariable> inputVariables;

        try
        {
            if (spvReflectCreateShaderModule(dataLength, data, &shaderModule) != SPV_REFLECT_RESULT_SUCCESS)
            {
                throw InvalidShaderCodeError("Unable to get reflection data for shader code");
            }
            shaderStage = getShaderStage(shaderModule);
            bindGroups = getBindings(shaderModule);
            inputVariables = getInputVariables(shaderModule);
        }
        catch (...)
        {
            spvReflectDestroyShaderModule(&shaderModule);
            throw;
        }

        spvReflectDestroyShaderModule(&shaderModule);

        return ShaderMetaData(shaderStage,std::move(bindGroups),std::move(inputVariables));
    }
} // slag
