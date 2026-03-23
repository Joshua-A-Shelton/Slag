#include "Descriptors.h"

#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    StructMember::StructMember(const std::string& name, GraphicsType type, uint32_t arrayLength, uint64_t relativeOffset,
        uint64_t totalOffset)
    {
        SLAG_ASSERT(type != GraphicsType::UNKNOWN && "Cannot create a struct member with an unknown type");
        SLAG_ASSERT(type != GraphicsType::STRUCT && "Use alternate constructor for creating struct members which are not primitive types");
        SLAG_ASSERT(arrayLength > 0 && "Array Length must be greater than zero (use 1 for a singular element)");
        _name = name;
        _type = type;
        _arrayLength = arrayLength;
        _relativeOffset = relativeOffset;
        _totalOffset = totalOffset;
    }

    StructMember::StructMember(const std::string& name, std::vector<StructMember>&& children, uint32_t arrayLength,
        uint64_t relativeOffset, uint64_t totalOffset)
    {
        SLAG_ASSERT(arrayLength > 0 && "Array Length must be greater than zero (use 1 for a singular element)");
        _name = name;
        _type = GraphicsType::STRUCT;
        _children = std::move(children);
        _arrayLength = arrayLength;
        _relativeOffset = relativeOffset;
        _totalOffset = totalOffset;
    }

    StructMember::StructMember(const StructMember& from)
    {
        copy(from);
    }

    StructMember& StructMember::operator=(const StructMember& from)
    {
        copy(from);
        return *this;
    }

    StructMember::StructMember(StructMember&& from) noexcept
    {
        move(from);
    }

    StructMember& StructMember::operator=(StructMember&& from) noexcept
    {
        move(from);
        return *this;
    }

    uint32_t StructMember::childCount() const
    {
        return _children.size();
    }

    const StructMember& StructMember::child(uint32_t index) const
    {
        return _children[index];
    }

    const std::string& StructMember::name() const
    {
        return _name;
    }

    GraphicsType StructMember::type() const
    {
        return _type;
    }

    uint64_t StructMember::relativeOffset() const
    {
        return _relativeOffset;
    }

    uint64_t StructMember::totalOffset() const
    {
        return _totalOffset;
    }

    uint32_t StructMember::arrayLength() const
    {
        return _arrayLength;
    }

    void StructMember::move(StructMember& from)
    {
        _children.swap(from._children);
        _name.swap(from._name);
        _relativeOffset = from._relativeOffset;
        _totalOffset = from._totalOffset;
        _arrayLength = from._arrayLength;
        _type = from._type;
    }

    void StructMember::copy(const StructMember& from)
    {
        _children = from._children;
        _name = from._name;
        _relativeOffset = from._relativeOffset;
        _totalOffset = from._totalOffset;
        _arrayLength = from._arrayLength;
        _type = from._type;
    }

    BufferLayout::BufferLayout(std::vector<StructMember>&& members)
    {
        _members = std::move(members);
    }

    BufferLayout::BufferLayout(const BufferLayout& from)
    {
        copy(from);
    }

    BufferLayout& BufferLayout::operator=(const BufferLayout& from)
    {
        copy(from);
        return *this;
    }

    BufferLayout::BufferLayout(BufferLayout&& from) noexcept
    {
        move(from);
    }

    BufferLayout& BufferLayout::operator=(BufferLayout&& from) noexcept
    {
        move(from);
        return *this;
    }

    uint32_t BufferLayout::memberCount() const
    {
        return _members.size();
    }

    const StructMember& BufferLayout::member(uint32_t index) const
    {
        return _members[index];
    }

    void BufferLayout::move(BufferLayout& from)
    {
        _members.swap(from._members);
    }

    void BufferLayout::copy(const BufferLayout& from)
    {
        _members = from._members;
    }

    DescriptorMeta::DescriptorMeta(const std::string& name, uint32_t setOffset, DescriptorType type,
                                   const SamplerDescription& samplerDescription)
    {
        SLAG_ASSERT(type == DescriptorType::SAMPLER && "type must be DescriptorType::SAMPLER when providing sampler description");
        _name = name;
        _setOffset = setOffset;
        _type = type;
        _details = samplerDescription;

    }

    DescriptorMeta::DescriptorMeta(const std::string& name, uint32_t setOffset, DescriptorType type,
        const TextureDescription& textureDescription)
    {
        SLAG_ASSERT((type == DescriptorType::SAMPLED_TEXTURE || type == DescriptorType::UNORDERED_ACCESS_TEXTURE) && "type must be DescriptorType::SAMPLED_TEXTURE or DescriptorType::UNORDERED_ACCESS_TEXTURE when providing texture description");
        _name = name;
        _setOffset = setOffset;
        _type = type;
        _details = textureDescription;
    }

    DescriptorMeta::DescriptorMeta(const std::string& name, uint32_t setOffset, DescriptorType type,
        BufferLayout&& bufferLayout)
    {
        SLAG_ASSERT((type == DescriptorType::UNIFORM_BUFFER || type == DescriptorType::UNORDERED_ACCESS_BUFFER) && "type must be DescriptorType::UNIFORM_BUFFER or DescriptorType::UNORDERED_ACCESS_BUFFER when providing buffer description");
        _name = name;
        _setOffset = setOffset;
        _type = type;
        _details = std::move(bufferLayout);
    }

    DescriptorMeta::DescriptorMeta(const std::string& name, uint32_t setOffset, DescriptorType type,
        const PixelFormat& pixelFormat)
    {
        SLAG_ASSERT((type == DescriptorType::UNIFORM_TEXEL_BUFFER || type == DescriptorType::UNORDERED_ACCESS_TEXEL_BUFFER) && "type must be DescriptorType::UNIFORM_TEXEL_BUFFER or DescriptorType::UNORDERED_ACCESS_TEXEL_BUFFER when providing a PixelFormat");
        _name = name;
        _setOffset = setOffset;
        _type = type;
        _details = pixelFormat;
    }

    DescriptorMeta::DescriptorMeta(const std::string& name, uint32_t setOffset, DescriptorType type,
        const AccelerationStructureDescription& accelerationDescription)
    {
        SLAG_ASSERT((type == DescriptorType::ACCELERATION_STRUCTURE) && "type must be DescriptorType::UNIFORM_TEXEL_BUFFER or DescriptorType::UNORDERED_ACCESS_TEXEL_BUFFER when providing a PixelFormat");
        _name = name;
        _setOffset = setOffset;
        _type = type;
        _details = accelerationDescription;
    }

    DescriptorMeta::DescriptorMeta(const DescriptorMeta& from)
    {
        copy(from);
    }

    DescriptorMeta& DescriptorMeta::operator=(const DescriptorMeta& from)
    {
        copy(from);
        return *this;
    }

    DescriptorMeta::DescriptorMeta(DescriptorMeta&& from) noexcept
    {
        move(from);
    }

    DescriptorMeta& DescriptorMeta::operator=(DescriptorMeta&& from) noexcept
    {
        move(from);
        return *this;
    }

    const std::string& DescriptorMeta::name() const
    {
        return _name;
    }

    uint32_t DescriptorMeta::setOffset() const
    {
        return _setOffset;
    }

    DescriptorType DescriptorMeta::type() const
    {
        return _type;
    }

    const SamplerDescription* DescriptorMeta::samplerDetails() const
    {
        return std::get_if<SamplerDescription>(&_details);
    }

    const TextureDescription* DescriptorMeta::textureDetails() const
    {
        return std::get_if<TextureDescription>(&_details);
    }

    const BufferLayout* DescriptorMeta::bufferDetails() const
    {
        return std::get_if<BufferLayout>(&_details);
    }

    const PixelFormat* DescriptorMeta::texelBufferDetails() const
    {
        return std::get_if<PixelFormat>(&_details);
    }

    const AccelerationStructureDescription* DescriptorMeta::accelerationStructureDetails() const
    {
        return std::get_if<AccelerationStructureDescription>(&_details);
    }


    void DescriptorMeta::move(DescriptorMeta& from)
    {
        _name.swap(from._name);
        _setOffset = from._setOffset;
        _type = from._type;
        _details.swap(from._details);
    }

    void DescriptorMeta::copy(const DescriptorMeta& from)
    {
        _name = from._name;
        _setOffset = from._setOffset;
        _type = from._type;
        _details = from._details;
    }
}
