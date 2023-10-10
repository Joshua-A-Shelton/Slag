#include <stdexcept>
#include <algorithm>
#include <cassert>
#include "VulkanUniform.h"

namespace slag
{
    namespace vulkan
    {
        VulkanUniform::VulkanUniform(SpvReflectDescriptorBinding* binding, VkShaderStageFlagBits shaderStage)
        {
            _binding = binding->binding;
            _descriptorType = static_cast<VkDescriptorType>(binding->descriptor_type);
            if(_descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
            {
                _descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            }
            else if(_descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
            {
                _descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
            }
            _name = binding->name;
            _accessibleFrom = shaderStage;

            auto& description = *binding->type_description;
            uint32_t offsetLocation = 0;
            for(int i=0; i< description.member_count; i++)
            {
                auto& descMember = description.members[i];
                if(descMember.type_flags & SPV_REFLECT_TYPE_FLAG_STRUCT)
                {
                    throw std::runtime_error("Shaders do not support nested structs");
                }
                if(descMember.traits.array.dims_count>0)
                {
                    throw std::runtime_error("Shaders do not support arrays");
                }

                std::string memberName;
                if(descMember.struct_member_name!= nullptr)
                {
                    memberName = descMember.struct_member_name;
                }
                else
                {
                    memberName = descMember.type_name;
                }

                uint16_t type = 0;
                if(descMember.type_flags & SPV_REFLECT_TYPE_FLAG_BOOL)
                {
                   type = GraphicsTypes::GraphicsType::BOOLEAN;
                }
                else if(descMember.type_flags & SPV_REFLECT_TYPE_FLAG_INT)
                {
                    if(descMember.traits.numeric.scalar.signedness)
                    {
                        type = GraphicsTypes::GraphicsType::UNSIGNED_INTEGER;
                    }
                    else
                    {
                        type = GraphicsTypes::GraphicsType::INTEGER;
                    }
                }
                else if(descMember.type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT)
                {
                    if(descMember.traits.numeric.scalar.width == 64)
                    {
                        type = GraphicsTypes::GraphicsType::DOUBLE;
                    }
                    else
                    {
                        type = GraphicsTypes::GraphicsType::FLOAT;
                    }
                }

                if(descMember.type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX)
                {
                    switch (descMember.traits.numeric.matrix.column_count)
                    {
                        case 2:
                            type |= GraphicsTypes::MATRIX2N_BIT;
                        case 3:
                            type |= GraphicsTypes::MATRIX3N_BIT;
                        case 4:
                            type |= GraphicsTypes::MATRIX3N_BIT;
                    }
                    switch (descMember.traits.numeric.matrix.row_count)
                    {
                        case 2:
                            type |= GraphicsTypes::VECTOR2_BIT;
                        case 3:
                            type |= GraphicsTypes::VECTOR3_BIT;
                        case 4:
                            type |= GraphicsTypes::VECTOR4_BIT;
                    }
                }
                else if(descMember.type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR)
                {
                    switch (descMember.traits.numeric.vector.component_count)
                    {
                        case 2:
                            type |= GraphicsTypes::VECTOR2_BIT;
                        case 3:
                            type |= GraphicsTypes::VECTOR3_BIT;
                        case 4:
                            type |= GraphicsTypes::VECTOR4_BIT;
                    }
                }

                //TODO: I'm not positive this is the right way to do alignment.... double check on this
                auto memberSize = GraphicsTypes::typeSize(static_cast<GraphicsTypes::GraphicsType>(type));
                auto paddedSize = GraphicsTypes::paddedTypeSize(static_cast<GraphicsTypes::GraphicsType>(type));
                auto additionalPadding = (offsetLocation+paddedSize) % paddedSize;
                offsetLocation+=additionalPadding;

                _descriptors.push_back(UniformDescriptor(i,offsetLocation,memberSize,static_cast<GraphicsTypes::GraphicsType>(type),memberName));
                offsetLocation+=memberSize;
            }
            _bufferSize = offsetLocation;
        }

        Uniform::UniformType VulkanUniform::uniformType()
        {
            switch (_descriptorType)
            {
                case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                    return UniformType::TEXTURE;
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                    return UniformType::STORAGE;
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                    return UniformType::UNIFORM;
            }
            assert(false && "Unimplemented");
            return UniformType::UNIFORM;
        }

        size_t VulkanUniform::descriptorCount()
        {
            return _descriptors.size();
        }

        const std::string& VulkanUniform::name()
        {
            return _name;
        }

        uint32_t VulkanUniform::binding()
        {
            return _binding;
        }

        void VulkanUniform::merge(VulkanUniform&& with)
        {
            _accessibleFrom = static_cast<VkShaderStageFlagBits>(_accessibleFrom | with._accessibleFrom);
            size_t memberOffsetIndex = 0;
            size_t originalSize = _descriptors.size();
            for(auto& mergeMember: with._descriptors)
            {
                if(memberOffsetIndex < originalSize)
                {
                    //same member or sub member
                    if (_descriptors[memberOffsetIndex].offset() >= mergeMember.offset() && _descriptors[memberOffsetIndex].offset()+_descriptors[memberOffsetIndex].size())
                    {
                        memberOffsetIndex++;
                        continue;
                    }
                    else if(memberOffsetIndex < originalSize-1)
                    {
                        memberOffsetIndex++;
                        //is member in padding
                        if (_descriptors[memberOffsetIndex].offset() > mergeMember.offset())
                        {
                            _descriptors.push_back(std::move(mergeMember));
                        }
                    }
                        //is member in padding, we've gone beyond the original uniform
                    else
                    {
                        _descriptors.push_back(std::move(mergeMember));
                    }
                    memberOffsetIndex++;
                }
                else
                {
                    _descriptors.push_back(std::move(mergeMember));
                }
            }

            _bufferSize = std::max(_bufferSize,with._bufferSize);
            std::sort(_descriptors.begin(), _descriptors.end(), UniformDescriptor::compareBinding);
        }

        VkDescriptorSetLayoutBinding VulkanUniform::vulkanBindingInfo()
        {
            VkDescriptorSetLayoutBinding vulkanBinding{};
            vulkanBinding.binding = _binding;
            vulkanBinding.stageFlags = _accessibleFrom;
            vulkanBinding.descriptorType = _descriptorType;
            vulkanBinding.descriptorCount = _descriptors.size();
            if(vulkanBinding.descriptorCount == 0)
            {
                vulkanBinding.descriptorCount = 1;
            }
            vulkanBinding.pImmutableSamplers = nullptr;
            return vulkanBinding;
        }

        bool VulkanUniform::compareBinding(Uniform &uniform1, Uniform &uniform2)
        {
            return uniform1.binding() < uniform2.binding();
        }

        VkDescriptorType VulkanUniform::vulkanDescriptorType()
        {
            return _descriptorType;
        }
    } // slag
} // vulkan