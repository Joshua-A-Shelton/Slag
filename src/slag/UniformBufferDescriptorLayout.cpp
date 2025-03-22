#include <stdexcept>
#include "UniformBufferDescriptorLayout.h"

namespace slag
{
    UniformBufferDescriptorLayout::UniformBufferDescriptorLayout(const std::string& name, GraphicsTypes::GraphicsType type, uint32_t arrayDepth, std::vector<UniformBufferDescriptorLayout>&& children, size_t size, size_t offset, size_t absoluteOffset)
    {
        _name = name;
        _type = type;
        _arrayDepth = arrayDepth;
        _children = std::move(children);
        _size = size;
        _offset = offset;
        _absoluteOffset = absoluteOffset;
    }

    UniformBufferDescriptorLayout::UniformBufferDescriptorLayout(UniformBufferDescriptorLayout&& from)
    {
        move(std::move(from));
    }

    UniformBufferDescriptorLayout& UniformBufferDescriptorLayout::operator=(UniformBufferDescriptorLayout&& from)
    {
        move(std::move(from));
        return *this;
    }

    void UniformBufferDescriptorLayout::move(UniformBufferDescriptorLayout&& from)
    {
        _name.swap(from._name);
        _arrayDepth = from._arrayDepth;
        _children.swap(from._children);
        _size = from.size();
        _offset = from.offset();
        _type = from.type();
    }

    const std::string& UniformBufferDescriptorLayout::name() const
    {
        return _name;
    }
    GraphicsTypes::GraphicsType UniformBufferDescriptorLayout::type()const
    {
        return _type;
    }

    size_t UniformBufferDescriptorLayout::childrenCount()const
    {
        return _children.size();
    }

    size_t UniformBufferDescriptorLayout::size()const
    {
        return _size;
    }

    const UniformBufferDescriptorLayout& UniformBufferDescriptorLayout::operator[](size_t index) const
    {
        return _children[index];
    }

    size_t UniformBufferDescriptorLayout::offset() const
    {
        return _offset;
    }

    size_t UniformBufferDescriptorLayout::absoluteOffset() const
    {
        return _absoluteOffset;
    }
} // slag