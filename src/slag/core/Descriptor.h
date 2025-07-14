#ifndef SLAG_DESCRIPTOR_H
#define SLAG_DESCRIPTOR_H

#include <string>

#include "GraphicsTypes.h"
#include "ShaderPipeline.h"

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
            ///Represents a section of a buffer that contains arbitrary data that both read and write operations can be performed on in the same shader (Generally GPU shaders or unsized arrays in pixel shaders)
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
            ShaderStage::Flags visibleStages = ShaderStage::VERTEX;

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
        Descriptor(const std::string& name, DescriptorType type, uint32_t arrayDepth, uint32_t binding, ShaderStage::Flags visibleStages);
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
        void move(Descriptor& from);
        std::string _name;
        Shape _shape;

    };

     ///Describes the structure for data in a uniform buffer
    class UniformBufferDescriptorLayout
    {
    public:
        /**
         * Create a UniformBufferDescriptorLayout, will assume all parameters are correct without checking. This will allow manual building, but requires the user to keep track of the details when creating the children's offset and sizes
         * @param name
         * @param type
         * @param arrayDepth
         * @param children
         * @param size
         * @param offset
         */
        UniformBufferDescriptorLayout(const std::string& name, GraphicsTypes::GraphicsType type, uint32_t arrayDepth, std::vector<UniformBufferDescriptorLayout>&& children, size_t size, size_t offset, size_t absoluteOffset);
        UniformBufferDescriptorLayout()=delete;
        UniformBufferDescriptorLayout(const UniformBufferDescriptorLayout&)=delete;
        UniformBufferDescriptorLayout& operator=(const UniformBufferDescriptorLayout&)=delete;
        UniformBufferDescriptorLayout(UniformBufferDescriptorLayout&& from);
        UniformBufferDescriptorLayout& operator=(UniformBufferDescriptorLayout&& from);
        ///The descriptive name of the object in the buffer, may be empty string
        const std::string& name()const;
        ///The type this layout represents
        GraphicsTypes::GraphicsType type()const;
        ///How many children layouts this layout has. Zero for non-struct layouts
        size_t childrenCount()const;
        ///Size in bytes this layout takes in the memory buffer
        size_t size()const;
        ///The offset position in bytes from the parent of this descriptor
        size_t offset()const;
        ///The offset from the beginning of the buffer
        size_t absoluteOffset()const;
        const UniformBufferDescriptorLayout& operator[](size_t index)const;
    private:
        void move(UniformBufferDescriptorLayout&& from);
        std::string _name;
        GraphicsTypes::GraphicsType _type= GraphicsTypes::STRUCT;
        uint32_t _arrayDepth = 1;
        std::vector<UniformBufferDescriptorLayout> _children;
        size_t _size = 0;
        size_t _offset=0;
        size_t _absoluteOffset=0;

    };

} // slag

#endif //SLAG_DESCRIPTOR_H
