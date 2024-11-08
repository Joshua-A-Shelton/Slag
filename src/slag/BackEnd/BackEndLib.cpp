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
    }
}