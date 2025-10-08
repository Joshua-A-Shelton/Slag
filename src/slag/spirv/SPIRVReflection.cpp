#include "SPIRVReflection.h"

#include <functional>
#include <spirv_reflect.h>
#include <unordered_map>

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

        BufferLayout bufferDescriptorLayoutFromSPV(SpvReflectBlockVariable* block)
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

        struct DescriptorGroupReflectionStub
        {
            uint32_t index=0;
            std::unordered_map<uint32_t, Descriptor> descriptors;
        };
        SPVReflectionData getReflectionData(ShaderCode** shaders, size_t shaderCount,std::string(*rename)(const DescriptorRenameParameters&,void*), void* renameData)
        {
            uint32_t totalSets = 0;
            std::unordered_map<uint32_t, DescriptorGroupReflectionStub> groups;
            std::unordered_map<uint32_t,std::unordered_map<uint32_t,BufferLayout>> bufferLayouts;
            std::unordered_map<uint32_t,std::unordered_map<uint32_t,TexelBufferDescription>> texelBufferDescriptions;

            uint32_t dimX = 0;
            uint32_t dimY = 0;
            uint32_t dimZ = 0;

            for (size_t i = 0; i < shaderCount; i++)
            {
                ShaderCode* shader = shaders[i];
                SLAG_ASSERT(shader->language() == ShaderCode::CodeLanguage::SPIRV && "Attempted to reflect spirv variables on non spirv code");
                SpvReflectShaderModule module;
                if (spvReflectCreateShaderModule(shader->dataSize(),shader->data(),&module)== SPV_REFLECT_RESULT_SUCCESS)
                {
                    if (shader->stage() == ShaderStageFlags::COMPUTE)
                    {
                        SLAG_ASSERT(dimX == 0 && dimY == 0 && dimZ == 0 && "Multiple Compute Stages are defined");
                        auto localSize = module.entry_points->local_size;
                        dimX = localSize.x;
                        dimY = localSize.y;
                        dimZ = localSize.z;
                    }
                    for (auto i=0; i< module.descriptor_set_count; i++)
                    {
                        auto& set = module.descriptor_sets[i];
                        if (set.set > totalSets)
                        {
                            totalSets = set.set;
                        }
                        auto group = groups.find(set.set);
                        if (group == groups.end())
                        {
                            group = groups.insert(std::pair<uint32_t, DescriptorGroupReflectionStub>(set.set, DescriptorGroupReflectionStub{})).first;
                            group->second.index = set.set;
                        }
                        auto& descriptorReflection = group->second;
                        for (auto i=0; i< set.binding_count; i++)
                        {
                            auto& binding = set.bindings[i];
                            auto descriptor = descriptorReflection.descriptors.find(binding->binding);
                            if (descriptor == descriptorReflection.descriptors.end())
                            {
                                std::string name = binding->name;
                                auto type = descriptorTypeFromSPV(binding->descriptor_type);
                                auto dimension = dimensionFromSPV(binding->image.dim);
                                if (rename!=nullptr)
                                {
                                    //TODO: probably should provide some level of data instead of nullptr to help with identification
                                    DescriptorRenameParameters renameParameters{};
                                    renameParameters.language = ShaderCode::CodeLanguage::SPIRV;
                                    renameParameters.originalName = name;
                                    renameParameters.descriptorGroupIndex = set.set;
                                    renameParameters.type = type;
                                    renameParameters.dimension = dimension;
                                    renameParameters.arrayDepth = binding->count;
                                    renameParameters.platformSpecificBindingIndex = binding->binding;
                                    renameParameters.platformData = binding;
                                    name = rename(renameParameters,renameData);
                                }
                                descriptor = descriptorReflection.descriptors.insert(std::pair<uint32_t,Descriptor>(binding->binding,Descriptor(name,type,dimension,binding->count,shader->stage()))).first;
                            }
                            else
                            {
                                auto& reflectedDescriptor = descriptor->second;
                                //scary const casts, but necessary to edit data we're generally not supposed to
                                auto& shape = const_cast<Descriptor::Shape&>(reflectedDescriptor.shape());
                                if (shape.type != descriptorTypeFromSPV(binding->descriptor_type) || shape.arrayDepth != binding->count)
                                {
                                    throw std::runtime_error(std::string("Shader stages contain incompatible descriptor groups: Group "+std::to_string(set.set))+" Binding "+std::to_string(binding->binding));
                                }
                                shape.visibleStages |= shader->stage();
                            }
                            auto descriptorType = descriptor->second.shape().type;
                            if (descriptorType == Descriptor::Type::UNIFORM_BUFFER || descriptorType == Descriptor::Type::STORAGE_BUFFER)
                            {
                                auto bufferDescription = bufferLayouts.find(set.set);
                                if ( bufferDescription == bufferLayouts.end())
                                {
                                    bufferDescription = bufferLayouts.insert(std::pair<uint32_t,std::unordered_map<uint32_t,BufferLayout>>(set.set,std::unordered_map<uint32_t,BufferLayout>{})).first;
                                    bufferDescription->second.insert(std::pair<uint32_t,BufferLayout>(binding->binding,bufferDescriptorLayoutFromSPV(&binding->block)));
                                }
                                else
                                {
                                    auto description = bufferDescription->second.find(binding->binding);
                                    if (description == bufferDescription->second.end())
                                    {
                                        bufferDescription->second.insert(std::pair<uint32_t,BufferLayout>(binding->binding,bufferDescriptorLayoutFromSPV(&binding->block)));
                                    }
                                    else
                                    {
                                        auto block = bufferDescriptorLayoutFromSPV(&binding->block);
                                        auto match = BufferLayout::compatible(block,description->second);
                                        if (match == 0)
                                        {
                                            throw std::runtime_error(std::string("Uniform buffer layout is incompatible across stages Set: ")+std::to_string(set.set)+" Binding: "+std::to_string(binding->binding));
                                        }
                                        else if (match == -1)
                                        {
                                            bufferDescription->second[binding->binding] = BufferLayout::merge(block,description->second);// std::move(block);
                                        }
                                        else
                                        {
                                            bufferDescription->second[binding->binding] = BufferLayout::merge(description->second,block);

                                        }
                                    }
                                }
                            }
                            else if (descriptorType == Descriptor::Type::UNIFORM_TEXEL_BUFFER || descriptorType == Descriptor::Type::STORAGE_TEXEL_BUFFER)
                            {
                                auto bufferDescription = texelBufferDescriptions.find(set.set);
                                //only add a new one if it doesn't already exist
                                if ( bufferDescription == texelBufferDescriptions.end())
                                {
                                    bufferDescription = texelBufferDescriptions.insert(std::pair<uint32_t,std::unordered_map<uint32_t,TexelBufferDescription>>(set.set,std::unordered_map<uint32_t,TexelBufferDescription>{})).first;
                                    bufferDescription->second.insert(std::pair<uint32_t,TexelBufferDescription>(binding->binding,texelBufferDescriptorLayoutFromSPV(binding)));
                                }
                                else
                                {
                                    bufferDescription->second.insert(std::pair<uint32_t,TexelBufferDescription>(binding->binding,texelBufferDescriptorLayoutFromSPV(binding)));
                                }

                            }

                        }

                    }
                    spvReflectDestroyShaderModule(&module);
                }
                else
                {
                    throw std::runtime_error("Unable to retrieve reflection data from spirv code");
                }
            }
            SPVReflectionData reflectionData
            {
                .groups = std::vector<SPVDescriptorGroupReflectionData>(totalSets+1),
                .bufferLayouts = std::move(bufferLayouts),
                .texelBufferDescriptions = std::move(texelBufferDescriptions),
                .entryPointXDim=dimX,
                .entryPointYDim=dimY,
                .entryPointZDim=dimZ
            };
            for (auto& group : groups)
            {
                std::vector<Descriptor> descriptors(group.second.descriptors.size());
                for (auto& kvpair : group.second.descriptors)
                {
                    if (kvpair.first >= descriptors.size())
                    {
                        descriptors.resize(kvpair.first+1);
                    }
                    descriptors[kvpair.first] = kvpair.second;
                }
                reflectionData.groups[group.first] = {.groupIndex = group.first,.descriptors = std::move(descriptors)};
            }
            return reflectionData;
        }
    } // spirv
} // slag
