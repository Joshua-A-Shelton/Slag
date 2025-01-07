#ifndef SLAG_DESCRIPTOR_H
#define SLAG_DESCRIPTOR_H

#include <string>
#include <vector>
#include "Shader.h"

namespace slag
{
    ///Describes data shaders expect (shader parameters essentially) in order to execute
    class Descriptor
    {
    public:
        ///The type this descriptor describes
        enum DescriptorType
        {
            ///Object that selects what texels to select from texture (layer, mip, etc)
            SAMPLER,
            ///Texure that requires a corresponding sampler to read
            SAMPLED_TEXTURE,
            ///Object that encapsulates both the texture and it's corresponding sampler
            SAMPLER_AND_TEXTURE,
            ///Texture that can have both be read on write operations can be perfomed on in the same shader (Generally GPU shaders)
            STORAGE_TEXTURE,
            ///Tightly packed 1D array of texels that image sampling operations can be performed on
            UNIFORM_TEXEL_BUFFER,
            ///Tighly packed 1D array of texels that image both read and write operations can be performed on in the same shader (Generally GPU shaders)
            STORAGE_TEXEL_BUFFER,
            ///Represents a section of a buffer that contains arbitrary data
            UNIFORM_BUFFER,
            ///Represents a section of a buffer that contains arbitrary data that both read and write operations can be performed on in the same shader (Generally GPU shaders)
            STORAGE_BUFFER,
            ///Texture that can be used for framebuffer local operations
            INPUT_ATTACHMENT,
            ///Object that is used in ray tracing and intersection testing
            ACCELERATION_STRUCTURE
        };
        ///Describes the metadata of a descriptor
        struct Shape
        {
            ///The type of descriptor being described
            DescriptorType type=DescriptorType::UNIFORM_BUFFER;
            ///How many objects are being described in an array
            uint32_t arrayDepth=1;
            ///The corresponding index to be bound to in the shader
            uint32_t binding=0;
            ///What stages of the shader this descriptor is visible to
            ShaderStages visibleStages = ShaderStageFlags::VERTEX;

            bool operator == (const Shape& to)const;
            bool operator != (const Shape& to)const;
        };
        Descriptor()=default;
        /**
         *
         * @param name Name of the descriptor, only used semantically, may differ between identical descriptor groups
         * @param type Descriptor Type
         * @param arrayDepth How many elements does this descriptor describe
         * @param binding Corresponding index to be bound to in the shader
         * @param visibleStages Stages of the shader is descriptor is visible to
         */
        Descriptor(const std::string& name, DescriptorType type, uint32_t arrayDepth, uint32_t binding, ShaderStages visibleStages);
        ~Descriptor()=default;
        Descriptor(const Descriptor& from);
        Descriptor& operator=(const Descriptor& from);
        Descriptor(Descriptor&& from);
        Descriptor& operator=(Descriptor&& from);
        ///Name of the descriptor, only used semantically, may differ between identical descriptor groups
        const std::string& name()const;
        ///Metadata of the descriptor
        const Shape& shape()const;
        friend class DescriptorGroup;
    protected:
        void copy(const Descriptor& from);
        void move(Descriptor& uniform);
        std::string _name;
        Shape _shape;

    };

} // slag

#endif //SLAG_DESCRIPTOR_H
