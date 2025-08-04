#include "Descriptor.h"

namespace slag
{
    bool Descriptor::Shape::operator==(const Shape& to) const
    {
        return binding == to.binding && type == to.type && arrayDepth == to.arrayDepth && visibleStages == to.visibleStages;
    }

    bool Descriptor::Shape::operator!=(const Shape& to) const
    {
        return !(*this == to);
    }

    Descriptor::Descriptor(const std::string& name, Type type, uint32_t arrayDepth, uint32_t binding, ShaderStageFlags visibleStages):_shape(type,arrayDepth,binding,visibleStages)
    {
        _name = name;
    }

    Descriptor::Descriptor(const Descriptor& from)
    {
        copy(from);
    }

    Descriptor& Descriptor::operator=(const Descriptor& from)
    {
        copy(from);
        return *this;
    }

    Descriptor::Descriptor(Descriptor&& from)
    {
        move(from);
    }

    Descriptor& Descriptor::operator=(Descriptor&& from)
    {
        move(from);
        return *this;
    }

    const std::string& Descriptor::name() const
    {
        return _name;
    }

    const Descriptor::Shape& Descriptor::shape() const
    {
        return _shape;
    }

    void Descriptor::copy(const Descriptor& from)
    {
        _name = from._name;
        _shape = from._shape;
    }

    void Descriptor::move(Descriptor& from)
    {
        _name.swap(from._name);
        _shape = from._shape;

    }

    UniformBufferDescriptorLayout::UniformBufferDescriptorLayout(const std::string& name, GraphicsType type,
        uint32_t arrayDepth, std::vector<UniformBufferDescriptorLayout>&& children, size_t size, size_t offset,
        size_t absoluteOffset)
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
        move(from);
    }

    UniformBufferDescriptorLayout& UniformBufferDescriptorLayout::operator=(UniformBufferDescriptorLayout&& from)
    {
        move(from);
        return *this;
    }

    const std::string& UniformBufferDescriptorLayout::name() const
    {
        return _name;
    }

    GraphicsType UniformBufferDescriptorLayout::type() const
    {
        return _type;
    }

    size_t UniformBufferDescriptorLayout::childrenCount() const
    {
        return _children.size();
    }

    size_t UniformBufferDescriptorLayout::size() const
    {
        return _size;
    }

    size_t UniformBufferDescriptorLayout::offset() const
    {
        return _offset;
    }

    size_t UniformBufferDescriptorLayout::absoluteOffset() const
    {
        return _absoluteOffset;
    }

    const UniformBufferDescriptorLayout& UniformBufferDescriptorLayout::operator[](size_t index) const
    {
        return _children[index];
    }

    int UniformBufferDescriptorLayout::compatible(UniformBufferDescriptorLayout& a, UniformBufferDescriptorLayout& b)
    {
        UniformBufferDescriptorLayout* superset = nullptr;
        UniformBufferDescriptorLayout* subset = nullptr;
        if (a.size() >= b.size())
        {
            superset = &a;
            subset = &b;
        }
        else
        {
            superset = &b;
            subset = &a;
        }
        for (auto i=0; i<subset->_children.size(); i++)
        {
            if (!compatibleRecursive(superset->_children[i],subset->_children[i]))
            {
                return 0;
            }
        }
        if (superset == &a)
        {
            return -1;
        }
        return 1;
    }

    void UniformBufferDescriptorLayout::move(UniformBufferDescriptorLayout& from)
    {
        _name.swap(from._name);
        _type= from._type;
        _arrayDepth = from._arrayDepth;
        _children.swap(from._children);
        _size = from._size;
        _offset=from._offset;
        _absoluteOffset=from._absoluteOffset;
    }

    bool UniformBufferDescriptorLayout::compatibleRecursive(UniformBufferDescriptorLayout& a,UniformBufferDescriptorLayout& b)
    {
        if (a.type() == b.type())
        {
            if (a.type() == GraphicsType::STRUCT)
            {
                if (a.size() == b.size() && a.childrenCount()==b.childrenCount())
                {
                    for (auto i=0; i<a.childrenCount(); i++)
                    {
                        if (!compatibleRecursive(a._children[i],b._children[i]))
                        {
                            return false;
                        }
                    }
                    return true;
                }
                else
                {
                    return false;
                }
            }
            return true;
        }
        return false;
    }
} // slag
