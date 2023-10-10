#include <stdexcept>
#include "VulkanUniform.h"

namespace slag
{
    namespace vulkan
    {
        VulkanUniform::VulkanUniform(SpvReflectDescriptorBinding* binding)
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
            _accessibleFrom = inStage;

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
                uint32_t baseSize = 0;
                switch (member.type)
                {
                    case BOOLEAN:
                    case INT:
                    case U_INT:
                        baseSize = sizeof(int);
                        break;
                    case FLOAT:
                        baseSize = sizeof(float);
                        break;
                    case DOUBLE:
                        baseSize = sizeof(double);
                        break;
                }
                uint32_t cols = member.dimensionOneLength;
                if(cols == 3)
                {
                    cols = 4;
                }
                uint32_t rows = member.dimensionTwoLength;
                if(rows == 3)
                {
                    rows = 4;
                }
                auto requiredAlignment = baseSize * cols * rows;
                offsetLocation += offsetLocation % requiredAlignment;

                uint32_t membersize = baseSize * member.dimensionOneLength * member.dimensionTwoLength;
                offsetLocation += membersize;

                _members.push_back(UniformDescriptor(i,offsetLocation,membersize,static_cast<GraphicsTypes::GraphicsType>(type),memberName));
            }
            _bufferSize = offsetLocation;
        }
    } // slag
} // vulkan