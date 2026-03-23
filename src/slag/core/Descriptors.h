#ifndef SLAG_DESCRIPTORS_H
#define SLAG_DESCRIPTORS_H
#include <string>
#include <variant>
#include <vector>

#include "Texture.h"
#include "GraphicsTypes.h"

namespace slag
{
    ///What type a descriptor is describing
    enum class DescriptorType: uint32_t
    {
        ///Unknown type, generally means unused
        UNKNOWN = 0,
        ///Object that selects texels from a texture
        SAMPLER,
        ///Read only texture, very fast to access pixels, but must have a sampler that determines the pixel to read
        SAMPLED_TEXTURE,
        ///Texture that can both be read and written to in a single shader
        UNORDERED_ACCESS_TEXTURE,
        ///Read only buffer containing arbitrary structured data
        UNIFORM_BUFFER,
        ///Buffer that can both be read and written to in a single shader as an unbounded array of arbitrary data
        UNORDERED_ACCESS_BUFFER,
        ///Read only buffer containing data which can have GPU pixel format conversions automatically applied when accessing data
        UNIFORM_TEXEL_BUFFER,
        ///Buffer that can can be which can have GPU pixel format conversions automatically applied when reading or writing data
        UNORDERED_ACCESS_TEXEL_BUFFER,
        ///Object that is used in ray tracing and intersection testing
        ACCELERATION_STRUCTURE
    };

    struct SamplerDescription
    {
        TextureType type;
    };
    struct TextureDescription
    {
        TextureType type;
    };
    class StructMember
    {
    public:
        StructMember(const std::string& name, GraphicsType type, uint32_t arrayLength, uint64_t relativeOffset, uint64_t totalOffset);
        StructMember(const std::string& name, std::vector<StructMember>&& children, uint32_t arrayLength, uint64_t relativeOffset, uint64_t totalOffset);
        StructMember(const StructMember& from);
        StructMember& operator=(const StructMember& from);
        StructMember(StructMember&& from) noexcept;
        StructMember& operator=(StructMember&& from) noexcept;
        ~StructMember()=default;
        [[nodiscard]] uint32_t childCount() const;
        [[nodiscard]] const StructMember& child(uint32_t index) const;
        [[nodiscard]] const std::string& name() const;
        [[nodiscard]] GraphicsType type() const;
        [[nodiscard]] uint64_t relativeOffset() const;
        [[nodiscard]] uint64_t totalOffset() const;
        [[nodiscard]] uint32_t arrayLength() const;
    private:
        void move(StructMember& from);
        void copy(const StructMember& from);
        std::vector<StructMember> _children;
        std::string _name;
        uint64_t _relativeOffset=0;
        uint64_t _totalOffset=0;
        uint32_t _arrayLength=1;
        GraphicsType _type=GraphicsType::UNKNOWN;
    };
    class BufferLayout
    {
    public:
        explicit BufferLayout(std::vector<StructMember>&& members);
        BufferLayout(const BufferLayout& from);
        BufferLayout& operator=(const BufferLayout& from);
        BufferLayout(BufferLayout&& from) noexcept;
        BufferLayout& operator=(BufferLayout&& from) noexcept;
        ~BufferLayout()=default;
        [[nodiscard]] uint32_t memberCount() const;
        [[nodiscard]] const StructMember& member(uint32_t index) const;
    private:
        void move(BufferLayout& from);
        void copy(const BufferLayout& from);
        std::vector<StructMember> _members;
    };
    struct AccelerationStructureDescription{};

    class DescriptorMeta
    {
    public:
        DescriptorMeta(const std::string& name, uint32_t setOffset, DescriptorType type, const SamplerDescription& samplerDescription);
        DescriptorMeta(const std::string& name, uint32_t setOffset, DescriptorType type, const TextureDescription& textureDescription);
        DescriptorMeta(const std::string& name, uint32_t setOffset, DescriptorType type, BufferLayout&& bufferLayout);
        DescriptorMeta(const std::string& name, uint32_t setOffset, DescriptorType type, const PixelFormat& pixelFormat);
        DescriptorMeta(const std::string& name, uint32_t setOffset, DescriptorType type, const AccelerationStructureDescription& accelerationDescription);
        DescriptorMeta(const DescriptorMeta& from);
        DescriptorMeta& operator=(const DescriptorMeta& from);
        DescriptorMeta(DescriptorMeta&& from) noexcept;
        DescriptorMeta& operator=(DescriptorMeta&& from) noexcept;
        ~DescriptorMeta()=default;

        [[nodiscard]] const std::string& name()const;
        [[nodiscard]] uint32_t setOffset()const;
        [[nodiscard]] DescriptorType type()const;
        const SamplerDescription* samplerDetails()const;
        const TextureDescription* textureDetails()const;
        const BufferLayout* bufferDetails()const;
        const PixelFormat* texelBufferDetails()const;
        const AccelerationStructureDescription* accelerationStructureDetails()const;
    private:
        void move(DescriptorMeta& from);
        void copy(const DescriptorMeta& from);
        std::variant<SamplerDescription,TextureDescription,BufferLayout,PixelFormat,AccelerationStructureDescription> _details;
        std::string _name;
        DescriptorType _type = DescriptorType::UNKNOWN;
        uint32_t _setOffset = 0;

    };

}
#endif //SLAG_DESCRIPTORS_H