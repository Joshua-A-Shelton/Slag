#include "SPIRVReflection.h"
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
                return Descriptor::Type::SAMPLER_AND_TEXTURE;
            case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
                return Descriptor::Type::INPUT_ATTACHMENT;
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

        UniformBufferDescriptorLayout uniformBufferDescriptorLayoutFromSPV(SpvReflectBlockVariable* block)
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

        struct DescriptorGroupReflectionStub
        {
            uint32_t index=0;
            std::unordered_map<uint32_t, Descriptor> descriptors;
        };
        SPVReflectionData getReflectionData(ShaderCode** shaders, size_t shaderCount)
        {
            uint32_t totalSets = 0;
            std::unordered_map<uint32_t, DescriptorGroupReflectionStub> groups;
            std::unordered_map<uint32_t,std::unordered_map<uint32_t,UniformBufferDescriptorLayout>> uniformBufferLayouts;
            for (size_t i = 0; i < shaderCount; i++)
            {
                ShaderCode* shader = shaders[i];
                SLAG_ASSERT(shader->language() == ShaderCode::CodeLanguage::SPIRV && "Attempted to reflect spirv variables on non spirv code");
                SpvReflectShaderModule module;
                if (spvReflectCreateShaderModule(shader->dataSize(),shader->data(),&module)== SPV_REFLECT_RESULT_SUCCESS)
                {
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
                            auto descriptor = descriptorReflection.descriptors.find(i);
                            if (descriptor == descriptorReflection.descriptors.end())
                            {
                                descriptor = descriptorReflection.descriptors.insert(std::pair<uint32_t,Descriptor>(binding->binding,Descriptor(binding->name,descriptorTypeFromSPV(binding->descriptor_type),binding->count,binding->binding,shader->stage()))).first;
                            }
                            else
                            {
                                auto& reflectedDescriptor = descriptor->second;
                                //scary const casts, but necessary to edit data we're generally not supposed to
                                auto& shape = const_cast<Descriptor::Shape&>(reflectedDescriptor.shape());
                                if (shape.binding!= binding->binding || shape.type != descriptorTypeFromSPV(binding->descriptor_type) || shape.arrayDepth != binding->count)
                                {
                                    throw std::runtime_error(std::string("Shader stages contain incompatible descriptor groups: Group "+std::to_string(set.set))+" Binding "+std::to_string(binding->binding));
                                }
                                shape.visibleStages |= shader->stage();
                            }

                            if (descriptor->second.shape().type == Descriptor::Type::UNIFORM_BUFFER)
                            {
                                auto bufferDescription = uniformBufferLayouts.find(set.set);
                                if ( bufferDescription == uniformBufferLayouts.end())
                                {
                                    bufferDescription = uniformBufferLayouts.insert(std::pair<uint32_t,std::unordered_map<uint32_t,UniformBufferDescriptorLayout>>(set.set,std::unordered_map<uint32_t,UniformBufferDescriptorLayout>{})).first;
                                    bufferDescription->second.insert(std::pair<uint32_t,UniformBufferDescriptorLayout>(binding->binding,uniformBufferDescriptorLayoutFromSPV(&binding->block)));
                                }
                                else
                                {
                                    auto description = bufferDescription->second.find(binding->binding);
                                    if (description == bufferDescription->second.end())
                                    {
                                        bufferDescription->second.insert(std::pair<uint32_t,UniformBufferDescriptorLayout>(binding->binding,uniformBufferDescriptorLayoutFromSPV(&binding->block)));
                                    }
                                    else
                                    {
                                        auto block = uniformBufferDescriptorLayoutFromSPV(&binding->block);
                                        auto match = UniformBufferDescriptorLayout::compatible(block,description->second);
                                        if (match == 0)
                                        {
                                            throw std::runtime_error(std::string("Uniform buffer layout is incompatible across stages Set: ")+std::to_string(set.set)+" Binding: "+std::to_string(binding->binding));
                                        }
                                        else if (match == -1)
                                        {
                                            bufferDescription->second[binding->binding] = UniformBufferDescriptorLayout::merge(block,description->second);// std::move(block);
                                        }
                                        else
                                        {
                                            bufferDescription->second[binding->binding] = UniformBufferDescriptorLayout::merge(description->second,block);

                                        }
                                    }
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
                .bufferLayouts = std::move(uniformBufferLayouts)
            };
            for (auto& group : groups)
            {
                std::vector<Descriptor> descriptors(group.second.descriptors.size());
                for (auto& kvpair : group.second.descriptors)
                {
                    descriptors[kvpair.first] = kvpair.second;
                }
                reflectionData.groups[group.first] = {.groupIndex = group.first,.descriptors = std::move(descriptors)};
            }
            return reflectionData;
        }
    } // spirv
} // slag
