#include "BackEndLib.h"
namespace slag
{
    namespace lib
    {
        BackEndLib* currentBackEnd = nullptr;

        BackEndLib* BackEndLib::get()
        {
            return currentBackEnd;
        }

        void BackEndLib::set(BackEndLib* library)
        {
            currentBackEnd = library;
        }

        Descriptor::DescriptorType BackEndLib::descriptorTypeFromSPV(SpvReflectDescriptorType type)
        {
            switch (type)
            {
                case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
                    return Descriptor::DescriptorType::ACCELERATION_STRUCTURE;
                case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                    return Descriptor::DescriptorType::SAMPLER_AND_TEXTURE;
                case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
                    return Descriptor::DescriptorType::INPUT_ATTACHMENT;
                case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                    return Descriptor::DescriptorType::SAMPLED_TEXTURE;
                case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
                    return Descriptor::DescriptorType::SAMPLER;
                case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                    return Descriptor::DescriptorType::STORAGE_BUFFER;
                case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                    return Descriptor::DescriptorType::STORAGE_BUFFER;
                case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                    return Descriptor::DescriptorType::STORAGE_TEXTURE;
                case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                    return Descriptor::DescriptorType::STORAGE_TEXEL_BUFFER;
                case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                    return Descriptor::DescriptorType::UNIFORM_BUFFER;
                case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                    return Descriptor::DescriptorType::UNIFORM_BUFFER;
                case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
                    return Descriptor::DescriptorType::UNIFORM_TEXEL_BUFFER;
            }
            throw std::runtime_error("unable to convert spvRefvlectDescriptorType to slag::Descriptor::DescriptorType");
        }

        GraphicsTypes::GraphicsType BackEndLib::graphicsTypeFromSPV(SpvReflectFormat format)
        {
            switch (format)
            {
                case SpvReflectFormat::SPV_REFLECT_FORMAT_UNDEFINED:
                    return GraphicsTypes::GraphicsType::UNKNOWN;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R64_UINT:
                    return GraphicsTypes::GraphicsType::UNKNOWN;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R64_SINT:
                    return GraphicsTypes::GraphicsType::UNKNOWN;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R64_SFLOAT:
                    return GraphicsTypes::GraphicsType::DOUBLE;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R64G64_UINT:
                    return GraphicsTypes::GraphicsType::UNKNOWN;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R64G64_SINT:
                    return GraphicsTypes::GraphicsType::UNKNOWN;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R64G64_SFLOAT:
                    return GraphicsTypes::GraphicsType::DOUBLE_VECTOR2;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R64G64B64_UINT:
                    return GraphicsTypes::GraphicsType::UNKNOWN;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R64G64B64_SINT:
                    return GraphicsTypes::GraphicsType::UNKNOWN;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R64G64B64_SFLOAT:
                    return GraphicsTypes::GraphicsType::DOUBLE_VECTOR3;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R64G64B64A64_UINT:
                    return GraphicsTypes::GraphicsType::UNKNOWN;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R64G64B64A64_SINT:
                    return GraphicsTypes::GraphicsType::UNKNOWN;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R64G64B64A64_SFLOAT:
                    return GraphicsTypes::GraphicsType::DOUBLE_VECTOR4;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R32_UINT:
                    return GraphicsTypes::GraphicsType::UNSIGNED_INTEGER;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R32_SINT:
                    return GraphicsTypes::GraphicsType::INTEGER;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R32_SFLOAT:
                    return GraphicsTypes::GraphicsType::FLOAT;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32_UINT:
                    return GraphicsTypes::GraphicsType::UNSIGNED_INTEGER_VECTOR2;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32_SINT:
                    return GraphicsTypes::GraphicsType::INTEGER_VECTOR2;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32_SFLOAT:
                    return GraphicsTypes::GraphicsType::VECTOR2;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32_UINT:
                    return GraphicsTypes::GraphicsType::UNSIGNED_INTEGER_VECTOR3;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32_SINT:
                    return GraphicsTypes::GraphicsType::INTEGER_VECTOR3;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:
                    return GraphicsTypes::GraphicsType::VECTOR3;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32A32_UINT:
                    return GraphicsTypes::GraphicsType::UNSIGNED_INTEGER_VECTOR4;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32A32_SINT:
                    return GraphicsTypes::GraphicsType::INTEGER_VECTOR4;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:
                    return GraphicsTypes::GraphicsType::VECTOR4;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R16_UINT:
                    return GraphicsTypes::GraphicsType::UNKNOWN;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R16_SINT:
                    return GraphicsTypes::GraphicsType::UNKNOWN;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R16_SFLOAT:
                    return GraphicsTypes::GraphicsType::UNKNOWN;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R16G16_UINT:
                    return GraphicsTypes::GraphicsType::UNKNOWN;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R16G16_SINT:
                    return GraphicsTypes::GraphicsType::UNKNOWN;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R16G16_SFLOAT:
                    return GraphicsTypes::GraphicsType::UNKNOWN;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R16G16B16_UINT:
                    return GraphicsTypes::GraphicsType::UNKNOWN;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R16G16B16_SINT:
                    return GraphicsTypes::GraphicsType::UNKNOWN;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R16G16B16_SFLOAT:
                    return GraphicsTypes::GraphicsType::UNKNOWN;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R16G16B16A16_UINT:
                    return GraphicsTypes::GraphicsType::UNKNOWN;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R16G16B16A16_SINT:
                    return GraphicsTypes::GraphicsType::UNKNOWN;
                case SpvReflectFormat::SPV_REFLECT_FORMAT_R16G16B16A16_SFLOAT:
                    return GraphicsTypes::GraphicsType::UNKNOWN;
            }
            return GraphicsTypes::GraphicsType::UNKNOWN;
        }

        GraphicsTypes::GraphicsType BackEndLib::graphicsTypeFromSPV(SpvReflectTypeDescription* typeDescription)
        {
            if (typeDescription)
            {
                if (typeDescription->type_flags & SPV_REFLECT_TYPE_FLAG_STRUCT)
                {
                    return GraphicsTypes::GraphicsType::STRUCT;
                }

                auto vectorComponents = typeDescription->traits.numeric.vector.component_count;
                auto matrixColumns = typeDescription->traits.numeric.matrix.column_count;
                auto matrixRows = typeDescription->traits.numeric.matrix.row_count;
                auto numericSignedness = typeDescription->traits.numeric.scalar.signedness;
                auto numericSize = typeDescription->traits.numeric.scalar.width;

                bool boolType = typeDescription->type_flags & SPV_REFLECT_TYPE_FLAG_BOOL;
                bool intType = typeDescription->type_flags & SPV_REFLECT_TYPE_FLAG_INT;
                bool floatType = typeDescription->type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT;
                unsigned int returnType = 0;
                if (boolType)
                {
                    returnType|=GraphicsTypes::BOOLEAN_BIT;
                }
                else if (intType)
                {
                    if (numericSignedness)
                    {
                        returnType|=GraphicsTypes::INTEGER_BIT;
                    }
                    else
                    {
                        returnType|=GraphicsTypes::UNSIGNED_INTEGER_BIT;
                    }
                }
                else if (floatType)
                {
                    if (numericSize == 32)
                    {
                        returnType|=GraphicsTypes::FLOAT_BIT;
                    }
                    else
                    {
                        returnType|=GraphicsTypes::DOUBLE_BIT;
                    }
                }

                if (matrixColumns)
                {
                    switch (matrixColumns)
                    {
                    case 2:
                        returnType|=GraphicsTypes::MATRIX2N_BIT;
                        break;
                    case 3:
                        returnType|=GraphicsTypes::MATRIX3N_BIT;
                        break;
                    case 4:
                        returnType|=GraphicsTypes::MATRIX4N_BIT;
                        break;
                    }
                    switch (matrixRows)
                    {
                    case 2:
                        returnType|=GraphicsTypes::VECTOR2_BIT;
                        break;
                    case 3:
                        returnType|=GraphicsTypes::VECTOR3_BIT;
                        break;
                    case 4:
                        returnType|=GraphicsTypes::VECTOR4_BIT;
                        break;
                    }
                }
                else if (vectorComponents)
                {
                    switch (vectorComponents)
                    {
                    case 2:
                        returnType|=GraphicsTypes::VECTOR2_BIT;
                        break;
                    case 3:
                        returnType|=GraphicsTypes::VECTOR3_BIT;
                        break;
                    case 4:
                        returnType|=GraphicsTypes::VECTOR4_BIT;
                        break;
                    }
                }
                return (GraphicsTypes::GraphicsType)returnType;
            }
            return GraphicsTypes::UNKNOWN;
        }

        UniformBufferDescriptorLayout BackEndLib::uniformBufferDescriptorLayoutFromSPV(SpvReflectBlockVariable* block)
        {
            std::string name = block->name;
            auto type = graphicsTypeFromSPV(block->type_description);
            std::vector<UniformBufferDescriptorLayout> children;
            for (auto i=0; i< block->member_count; i++)
            {
                children.push_back(uniformBufferDescriptorLayoutFromSPV(block->members+i));
            }
            if (block->array.dims_count > 1)
            {
                throw std::invalid_argument("Invalid array dimensions");
            }
            auto dims = block->array.dims[0];
            auto arrayDepth = dims == 0? 1 : dims;
            return UniformBufferDescriptorLayout(name,type,arrayDepth,std::move(children),block->size,block->offset,block->absolute_offset);
        }
    }
}