#include "SPIRVReflection.h"
#include <functional>
#include <spirv_reflect.h>
#include <unordered_set>
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace spirv
    {

        GraphicsType graphicsTypeFromSPV(SpvReflectTypeDescription* typeDescription)
        {
            if (typeDescription)
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
                GraphicsTypeBits returnType = GraphicsTypeBits::NONE_BIT;
                if (boolType)
                {
                    returnType|=GraphicsTypeBits::BOOLEAN_BIT;
                }
                else if (intType)
                {
                    if (numericSignedness)
                    {
                        returnType|=GraphicsTypeBits::INTEGER_BIT;
                    }
                    else
                    {
                        returnType|=GraphicsTypeBits::UNSIGNED_INTEGER_BIT;
                    }
                }
                else if (floatType)
                {
                    if (numericSize == 32)
                    {
                        returnType|=GraphicsTypeBits::FLOAT_BIT;
                    }
                    else
                    {
                        returnType|=GraphicsTypeBits::DOUBLE_BIT;
                    }
                }

                if (matrixColumns)
                {
                    switch (matrixColumns)
                    {
                    case 2:
                        returnType|=GraphicsTypeBits::MATRIX2N_BIT;
                        break;
                    case 3:
                        returnType|=GraphicsTypeBits::MATRIX3N_BIT;
                        break;
                    case 4:
                        returnType|=GraphicsTypeBits::MATRIX4N_BIT;
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
            return GraphicsType::UNKNOWN;
        }
        Descriptor::Type descriptorTypeFromSPV(SpvReflectDescriptorType type)
        {
            switch (type)
            {
            case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
                return Descriptor::Type::ACCELERATION_STRUCTURE;
            case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                throw std::runtime_error("Combined Texture/Sampler descriptors are not supported");
            case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
                throw std::runtime_error("Input Attachment descriptors are not supported");
            case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                return Descriptor::Type::SAMPLED_TEXTURE;
            case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
                return Descriptor::Type::SAMPLER;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                return Descriptor::Type::STORAGE_BUFFER;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                return Descriptor::Type::STORAGE_BUFFER;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                return Descriptor::Type::STORAGE_TEXTURE;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                return Descriptor::Type::STORAGE_TEXEL_BUFFER;
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                return Descriptor::Type::UNIFORM_BUFFER;
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                return Descriptor::Type::UNIFORM_BUFFER;
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
                return Descriptor::Type::UNIFORM_TEXEL_BUFFER;
            }
            throw std::runtime_error("unable to convert spvRefvlectDescriptorType to slag::Descriptor::DescriptorType");
        }
        Descriptor::Dimension dimensionFromSPV(SpvDim dim)
        {
            switch (dim)
            {
            case SpvDim2D:
                return Descriptor::Dimension::TWO_DIMENSIONAL;
            case SpvDim3D:
                return Descriptor::Dimension::THREE_DIMENSIONAL;
            case SpvDimCube:
                return Descriptor::Dimension::CUBE;
            }
            return Descriptor::Dimension::ONE_DIMENSIONAL;
        }
        Pixels::Format pixelFormatFromSPV(SpvImageFormat format)
        {
            switch (format)
            {
            case SpvImageFormat::SpvImageFormatR8:
                return Pixels::Format::R8_UNORM;
                break;
            case SpvImageFormat::SpvImageFormatR8Snorm:
                return Pixels::Format::R8_SNORM;
                break;
            case SpvImageFormat::SpvImageFormatR11fG11fB10f:
                return Pixels::Format::R11G11B10_FLOAT;
                break;
            case SpvImageFormat::SpvImageFormatR8ui:
                return Pixels::Format::R8_UINT;
                break;
            case SpvImageFormat::SpvImageFormatR8i:
                return Pixels::Format::R8_SINT;
                break;
            case SpvImageFormat::SpvImageFormatR16:
                return Pixels::Format::R16_UNORM;
                break;
            case SpvImageFormat::SpvImageFormatR16f:
                return Pixels::Format::R16_FLOAT;
                break;
            case SpvImageFormat::SpvImageFormatR16i:
                return Pixels::Format::R16_SINT;
                break;
            case SpvImageFormat::SpvImageFormatR16Snorm:
                return Pixels::Format::R16_SNORM;
                break;
            case SpvImageFormat::SpvImageFormatR16ui:
                return Pixels::Format::R16_UINT;
                break;
            case SpvImageFormat::SpvImageFormatR32f:
                return Pixels::Format::R32_FLOAT;
                break;
            case SpvImageFormat::SpvImageFormatR32i:
                return Pixels::Format::R32_SINT;
                break;
            case SpvImageFormat::SpvImageFormatR32ui:
                return Pixels::Format::R32_UINT;
                break;
            case SpvImageFormat::SpvImageFormatR64i:
                return Pixels::Format::UNDEFINED;
                break;
            case SpvImageFormat::SpvImageFormatR64ui:
                return Pixels::Format::UNDEFINED;
                break;
            case SpvImageFormat::SpvImageFormatRg8:
                return Pixels::Format::R8G8_UNORM;
                break;
            case SpvImageFormat::SpvImageFormatRg8i:
                return Pixels::Format::R8G8_SINT;
                break;
            case SpvImageFormat::SpvImageFormatRg8Snorm:
                return Pixels::Format::R8G8_SNORM;
                break;
            case SpvImageFormat::SpvImageFormatRg8ui:
                return Pixels::Format::R8G8_UINT;
                break;
            case SpvImageFormat::SpvImageFormatRg16:
                return Pixels::Format::R16G16_UNORM;
                break;
            case SpvImageFormat::SpvImageFormatRg16f:
                return Pixels::Format::R16G16_FLOAT;
                break;
            case SpvImageFormat::SpvImageFormatRg16i:
                return Pixels::Format::R16G16_SINT;
                break;
            case SpvImageFormat::SpvImageFormatRg16Snorm:
                return Pixels::Format::R16G16_SNORM;
                break;
            case SpvImageFormat::SpvImageFormatRg16ui:
                return Pixels::Format::R16G16_UINT;
                break;
            case SpvImageFormat::SpvImageFormatRg32f:
                return Pixels::Format::R32G32_FLOAT;
                break;
            case SpvImageFormat::SpvImageFormatRg32i:
                return Pixels::Format::R32G32_SINT;
                break;
            case SpvImageFormat::SpvImageFormatRg32ui:
                return Pixels::Format::R32G32_UINT;
                break;
            case SpvImageFormat::SpvImageFormatRgb10A2:
                return Pixels::Format::R10G10B10A2_UNORM;
                break;
            case SpvImageFormat::SpvImageFormatRgb10a2ui:
                return Pixels::Format::R10G10B10A2_UINT;
                break;
            case SpvImageFormat::SpvImageFormatRgba8:
                return Pixels::Format::R8G8B8A8_UNORM;
                break;
            case SpvImageFormat::SpvImageFormatRgba8i:
                return Pixels::Format::R8G8B8A8_SINT;
                break;
            case SpvImageFormat::SpvImageFormatRgba8Snorm:
                return Pixels::Format::R8G8B8A8_SNORM;
                break;
            case SpvImageFormat::SpvImageFormatRgba8ui:
                return Pixels::Format::R8G8B8A8_UINT;
                break;
            case SpvImageFormat::SpvImageFormatRgba16:
                return Pixels::Format::R16G16B16A16_UNORM;
                break;
            case SpvImageFormat::SpvImageFormatRgba16f:
                return Pixels::Format::R16G16B16A16_FLOAT;
                break;
            case SpvImageFormat::SpvImageFormatRgba16i:
                return Pixels::Format::R16G16B16A16_SINT;
                break;
            case SpvImageFormat::SpvImageFormatRgba16Snorm:
                return Pixels::Format::R16G16B16A16_SNORM;
                break;
            case SpvImageFormat::SpvImageFormatRgba16ui:
                return Pixels::Format::R16G16B16A16_UINT;
                break;
            case SpvImageFormat::SpvImageFormatRgba32f:
                return Pixels::Format::R32G32B32A32_FLOAT;
                break;
            case SpvImageFormat::SpvImageFormatRgba32i:
                return Pixels::Format::R32G32B32A32_SINT;
                break;
            case SpvImageFormat::SpvImageFormatRgba32ui:
                return Pixels::Format::R32G32B32A32_UINT;
                break;
            }
            return Pixels::Format::UNDEFINED;
        }

        VertexInputAttribute inputFromSPV(const SpvReflectInterfaceVariable* variable)
        {
            std::string name;
            if (variable->name!=nullptr){name = variable->name;}
            auto type = graphicsTypeFromSPV(variable->type_description);
            auto dims = variable->array.dims[0];
            auto arrayDepth = dims == 0? 1 : dims;

            return VertexInputAttribute(name,type,arrayDepth,variable->location);
        }

        BufferLayout bufferDescriptorLayoutFromSPV(const SpvReflectBlockVariable* block)
        {
            std::string name;
            if (block->name!=nullptr){name = block->name;}
            auto type = graphicsTypeFromSPV(block->type_description);
            std::vector<BufferLayout> children;
            for (auto i=0; i< block->member_count; i++)
            {
                children.push_back(bufferDescriptorLayoutFromSPV(block->members+i));
            }
            if (block->array.dims_count > 1)
            {
                throw std::invalid_argument("Invalid array dimensions");
            }
            auto dims = block->array.dims[0];
            auto arrayDepth = dims == 0? 1 : dims;
            return BufferLayout(name,type,arrayDepth,std::move(children),block->size,block->offset,block->absolute_offset);
        }

        TexelBufferDescription texelBufferDescriptorLayoutFromSPV(SpvReflectDescriptorBinding* binding)
        {

            return TexelBufferDescription(pixelFormatFromSPV(binding->image.image_format));
        }

        DescriptorBindingGroup generateBindingGroup(SpvReflectDescriptorSet* group, ShaderStageFlags stage)
        {

            std::vector<DescriptorBinding> descriptors(group->binding_count);
            for (auto i=0; i< group->binding_count; i++)
            {
                auto currentBinding = group->bindings[i];
                Descriptor descriptor(currentBinding->name,descriptorTypeFromSPV(currentBinding->descriptor_type),dimensionFromSPV(currentBinding->image.dim),currentBinding->count,stage);
                descriptors[i] = DescriptorBinding(descriptor,currentBinding->binding);
            }
            return DescriptorBindingGroup(descriptors.data(),descriptors.size(),group->set);
        }

        ShaderMetaData reflectShaderCode(ShaderCode& shaderCode)
        {
            ShaderStageFlags stage = shaderCode.stage();
            std::vector<VertexInputAttribute> vertexInputs;
            std::vector<DescriptorBindingGroup> bindingGroups;
            std::vector<BufferDescriptorBindingLayout> uniformBufferLayouts;
            std::vector<BufferDescriptorBindingLayout> storageBufferLayouts;
            std::vector<TexelBufferDescriptorBinding> texelBufferDescriptions;
            BufferLayout pushConstantsLayout;
            uint32_t xComputeThreads = 0;
            uint32_t yComputeThreads = 0;
            uint32_t zComputeThreads = 0;

            SLAG_ASSERT(shaderCode.language() == ShaderCode::CodeLanguage::SPIRV && "Attempted to reflect spirv data on non spirv code");
            SpvReflectShaderModule module;
            if (spvReflectCreateShaderModule(shaderCode.dataSize(),shaderCode.data(),&module)== SPV_REFLECT_RESULT_SUCCESS)
            {
                if (shaderCode.stage() == ShaderStageFlags::COMPUTE)
                {
                    auto localSize = module.entry_points->local_size;
                    xComputeThreads = localSize.x;
                    yComputeThreads = localSize.y;
                    zComputeThreads = localSize.z;
                }
                else if (shaderCode.stage() == ShaderStageFlags::VERTEX)
                {
                    vertexInputs.resize(module.input_variable_count);
                    for (int j = 0; j < module.input_variable_count; j++)
                    {
                        auto inputVariable = module.input_variables[j];
                        vertexInputs[j]=inputFromSPV(inputVariable);
                    }
                }

                bindingGroups.resize(module.descriptor_set_count);
                for (auto i=0; i< module.descriptor_set_count; i++)
                {
                    auto& currentGroup = module.descriptor_sets[i];
                    auto& createdGroup = bindingGroups[i];
                    createdGroup = generateBindingGroup(&currentGroup,stage);

                    for (auto j=0; j<createdGroup.bindingCount(); j++)
                    {
                        auto& binding = createdGroup.descriptorBinding(j);
                        switch (binding.descriptor().shape().type)
                        {
                        case Descriptor::Type::UNIFORM_BUFFER:
                            uniformBufferLayouts.emplace_back(currentGroup.set,j,bufferDescriptorLayoutFromSPV(&currentGroup.bindings[j]->block));
                            break;
                        case Descriptor::Type::STORAGE_BUFFER:
                            storageBufferLayouts.emplace_back(currentGroup.set,j,bufferDescriptorLayoutFromSPV(&currentGroup.bindings[j]->block));
                            break;
                        case Descriptor::Type::UNIFORM_TEXEL_BUFFER:
                        case Descriptor::Type::STORAGE_TEXEL_BUFFER:
                            texelBufferDescriptions.emplace_back(currentGroup.set,j,texelBufferDescriptorLayoutFromSPV(currentGroup.bindings[j]));
                            break;
                        default:
                            break;
                        }
                    }
                }

                for (uint32_t blockIndex = 0; blockIndex < module.push_constant_block_count; ++blockIndex)
                {
                    SpvReflectResult result;
                    auto range = spvReflectGetPushConstantBlock(&module,blockIndex,&result);
                    if (pushConstantsLayout.type()==GraphicsType::UNKNOWN)
                    {
                        pushConstantsLayout = bufferDescriptorLayoutFromSPV(range);
                    }
                    else
                    {
                        pushConstantsLayout = BufferLayout::merge(pushConstantsLayout,bufferDescriptorLayoutFromSPV(range));
                    }
                }

            }
            else
            {
                throw std::runtime_error("Unable to reflect spirv code");
            }
            return ShaderMetaData(stage,std::move(vertexInputs),std::move(bindingGroups),std::move(uniformBufferLayouts),std::move(storageBufferLayouts),std::move(texelBufferDescriptions),pushConstantsLayout,xComputeThreads,yComputeThreads,zComputeThreads);

        }
    } // spirv
} // slag
