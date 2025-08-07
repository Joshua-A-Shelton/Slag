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

    UniformBufferDescriptorLayout::UniformBufferDescriptorLayout(const UniformBufferDescriptorLayout& from)
    {
        copy(from);
    }

    UniformBufferDescriptorLayout& UniformBufferDescriptorLayout::operator=(const UniformBufferDescriptorLayout& from)
    {
        copy(from);
        return *this;
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

    uint32_t UniformBufferDescriptorLayout::arrayDepth() const
    {
        return  _arrayDepth;
    }

    const UniformBufferDescriptorLayout& UniformBufferDescriptorLayout::child(size_t index)
    {
        return _children[index];
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

    UniformBufferDescriptorLayout UniformBufferDescriptorLayout::merge(const UniformBufferDescriptorLayout& superset, const UniformBufferDescriptorLayout& subset)
    {
        if (superset.arrayDepth() > 1 || subset.arrayDepth() > 1 ||
            superset.offset()!= superset.absoluteOffset() || subset.offset()!= subset.absoluteOffset() ||
            superset.offset()!=0 || subset.offset()!=0 ||
            superset.type() != GraphicsType::STRUCT || subset.type() != GraphicsType::STRUCT)
        {
            throw std::runtime_error("UniformBufferDescriptorLayouts can only be merged if both are top level struct types");
        }
        std::vector<UniformBufferDescriptorLayout> newSets;
        size_t supersetIndex = 0;
        size_t subsetIndex = 0;

        auto maxSize = std::max(superset.size(),subset.size());

        while (supersetIndex < superset.childrenCount() || subsetIndex < subset.childrenCount())
        {
            if (supersetIndex >= superset.childrenCount())
            {
                for (auto i= subsetIndex; i<subset.childrenCount(); i++)
                {
                    newSets.push_back(subset._children[i]);
                }
                break;
            }
            else if (subsetIndex >= subset.childrenCount())
            {
                for (auto i=supersetIndex; i<superset.childrenCount(); i++)
                {
                    newSets.push_back(superset._children[i]);
                }
                break;
            }

            auto& super = superset._children[supersetIndex];
            auto& sub = subset._children[subsetIndex];

            if (proceeds(super,sub))
            {
                newSets.push_back(super);
                supersetIndex++;
            }
            else if (proceeds(sub,super))
            {
                newSets.push_back(sub);
                subsetIndex++;
            }
            else if (encompasses(super,sub))
            {
                newSets.push_back(super);
                supersetIndex++;
                while ( subsetIndex < subset.childrenCount() && subset[subsetIndex].offset() < super.offset()+superset.childrenCount())
                {
                    subsetIndex++;
                }
            }
            else if (encompasses(sub,super))
            {
                newSets.push_back(sub);
                subsetIndex++;
                while (supersetIndex < superset.childrenCount() && superset[supersetIndex].offset() < sub.offset()+subset.childrenCount())
                {
                    supersetIndex++;
                }
            }
            else
            {
                throw std::runtime_error("Layouts are incompatible");
            }

            if (supersetIndex == super.childrenCount() && subsetIndex == subset.childrenCount())
            {
                break;
            }

        }

        return UniformBufferDescriptorLayout(superset.name(),GraphicsType::STRUCT,1,std::move(newSets),maxSize,0,0);


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

    void UniformBufferDescriptorLayout::copy(const UniformBufferDescriptorLayout& from)
    {
        _name = from._name;
        _type= from._type;
        _arrayDepth = from._arrayDepth;
        _children = from._children;
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

    bool UniformBufferDescriptorLayout::proceeds(const UniformBufferDescriptorLayout& a, const UniformBufferDescriptorLayout& b)
    {
        if (a.offset()+a.size()<=b.offset())
        {
            return true;
        }
        return false;
    }

    bool UniformBufferDescriptorLayout::encompasses(const UniformBufferDescriptorLayout& a, const UniformBufferDescriptorLayout& b)
    {
        if (a.offset() <= b.offset() && a.offset()+a.size() >= b.offset()+b.size())
        {
            return true;
        }
        return false;
    }
} // slag
