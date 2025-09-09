#ifndef SLAG_DESCRIPTOR_H
#define SLAG_DESCRIPTOR_H

#include <bit>
#include <string>

#include "GraphicsTypes.h"
#include "ShaderPipeline.h"
#include "Texture.h"

namespace slag
{
    ///Describes data shaders expect (shader parameters essentially) in order to execute
    class Descriptor
    {
    public:
        ///The type this descriptor describes
        enum class Type: uint16_t
        {
            ///Unknown descriptor, generally means unused
            UNKNOWN = 0,
            ///Object that selects what texels to select from texture (layer, mip, etc)
            SAMPLER,
            ///Texure that requires a corresponding sampler to read
            SAMPLED_TEXTURE,
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
            ///Object that is used in ray tracing and intersection testing
            ACCELERATION_STRUCTURE
        };
        ///Describes the metadata of a descriptor
        struct Shape
        {
            ///The type of descriptor being described
            Type type=Type::UNKNOWN;
            ///How many objects are being described in an array
            uint32_t arrayDepth=1;
            ///The corresponding index to be bound to in the shader
            uint32_t binding=0;
            ///What stages of the shader this descriptor is visible to
            ShaderStageFlags visibleStages = std::bit_cast<ShaderStageFlags>(uint16_t(0));

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
        Descriptor(const std::string& name, Type type, uint32_t arrayDepth, uint32_t binding, ShaderStageFlags visibleStages);
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

     ///Describes the structure for data in a buffer
    class BufferLayout
    {
    public:
        /**
         * Create a BufferDescriptorLayout, will assume all parameters are correct without checking. This will allow manual building, but requires the user to keep track of the details when creating the children's offset and sizes
         * @param name
         * @param type
         * @param arrayDepth
         * @param children
         * @param size
         * @param offset
         */
        BufferLayout(const std::string& name, GraphicsType type, uint32_t arrayDepth, std::vector<BufferLayout>&& children, size_t size, size_t offset, size_t absoluteOffset);
        ///Create an invalid uniform buffer descriptor layout
        BufferLayout()=default;
        BufferLayout(const BufferLayout& from);
        BufferLayout& operator=(const BufferLayout& from);
        BufferLayout(BufferLayout&& from);
        BufferLayout& operator=(BufferLayout&& from);
        ///The descriptive name of the object in the buffer, may be empty string
        const std::string& name()const;
        ///The type this layout represents
        GraphicsType type()const;
        ///How many children layouts this layout has. Zero for non-struct layouts
        size_t childrenCount()const;
        ///Size in bytes this layout takes in the memory buffer
        size_t size()const;
        ///The offset position in bytes from the parent of this descriptor
        size_t offset()const;
        ///The offset from the beginning of the buffer
        size_t absoluteOffset()const;
        ///Number of elements in array
        uint32_t arrayDepth()const;
        const BufferLayout& child(size_t index);
        const BufferLayout& operator[](size_t index)const;
        /**
         * Determines if two buffer layouts are compatible
         * @param a first layout
         * @param b second layout
         * @return -1 if 'a' is a super-set of 'b', 1 if 'b' is a super-set of 'a', or zero if they're incompatible
         */
        static int compatible(const BufferLayout& a, const BufferLayout& b);
        /**
         * Merge two Buffer Descriptor Layouts, where subset will fill in any un-named or untyped space in the superset
         * @param superset Bigger of the two layouts
         * @param subset Smaller of the two subsets
         * @return
         */
        static BufferLayout merge(const BufferLayout& superset, const BufferLayout& subset);
    private:
        void move(BufferLayout& from);
        void copy(const BufferLayout& from);
        static bool compatibleRecursive(const BufferLayout& a, const BufferLayout& b);
        static bool proceeds(const BufferLayout& a, const BufferLayout& b);
        static bool encompasses(const BufferLayout& a, const BufferLayout& b);
        std::string _name;
        GraphicsType _type= GraphicsType::UNKNOWN;
        uint32_t _arrayDepth = 1;
        std::vector<BufferLayout> _children;
        size_t _size = 0;
        size_t _offset=0;
        size_t _absoluteOffset=0;

    };

    ///Describes the data in a texel buffer
    class TexelBufferDescription
    {
    public:
        TexelBufferDescription(Pixels::Format format);
        Pixels::Format format()const;
    private:
        Pixels::Format _format;
    };

} // slag

#endif //SLAG_DESCRIPTOR_H
