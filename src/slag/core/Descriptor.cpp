#include "Descriptor.h"

#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    bool Descriptor::Shape::operator==(const Shape& to) const
    {
        return type == to.type && dimension == to.dimension && arrayDepth == to.arrayDepth && visibleStages == to.visibleStages;
    }

    bool Descriptor::Shape::operator!=(const Shape& to) const
    {
        return !(*this == to);
    }

    Descriptor::Descriptor(const std::string& name, Type type, Dimension dimension, uint32_t arrayDepth, ShaderStageFlags visibleStages):_shape(type,dimension,arrayDepth,visibleStages)
    {
        SLAG_ASSERT(arrayDepth > 0 && "Array depth must be greater than zero");
        SLAG_ASSERT((type == Type::UNKNOWN && dimension == Dimension::UNKNOWN) || (type != Type::UNKNOWN && dimension != Dimension::UNKNOWN) && "If type or dimension is unknown, both must be");
        SLAG_ASSERT(((type == Type::STORAGE_BUFFER || type == Type::STORAGE_TEXEL_BUFFER || type == Type::UNIFORM_BUFFER || type == Type::UNIFORM_TEXEL_BUFFER || type == Type::ACCELERATION_STRUCTURE || type == Type::SAMPLER) && dimension == Dimension::ONE_DIMENSIONAL) ||
            (type == Type::SAMPLED_TEXTURE || type == Type::STORAGE_TEXTURE) && "Only textures may be non-one dimensional");
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

    BufferLayout::BufferLayout(const std::string& name, GraphicsType type,
        uint32_t arrayDepth, std::vector<BufferLayout>&& children, size_t size, size_t offset,
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

    BufferLayout::BufferLayout(const BufferLayout& from)
    {
        copy(from);
    }

    BufferLayout& BufferLayout::operator=(const BufferLayout& from)
    {
        copy(from);
        return *this;
    }

    BufferLayout::BufferLayout(BufferLayout&& from)
    {
        move(from);
    }

    BufferLayout& BufferLayout::operator=(BufferLayout&& from)
    {
        move(from);
        return *this;
    }

    const std::string& BufferLayout::name() const
    {
        return _name;
    }

    GraphicsType BufferLayout::type() const
    {
        return _type;
    }

    size_t BufferLayout::childrenCount() const
    {
        return _children.size();
    }

    size_t BufferLayout::size() const
    {
        return _size;
    }

    size_t BufferLayout::offset() const
    {
        return _offset;
    }

    size_t BufferLayout::absoluteOffset() const
    {
        return _absoluteOffset;
    }

    uint32_t BufferLayout::arrayDepth() const
    {
        return  _arrayDepth;
    }

    const BufferLayout& BufferLayout::child(size_t index)
    {
        return _children[index];
    }

    const BufferLayout& BufferLayout::operator[](size_t index) const
    {
        return _children[index];
    }

    int BufferLayout::compatible(const BufferLayout& a, const BufferLayout& b)
    {
        const BufferLayout* superset = nullptr;
        const BufferLayout* subset = nullptr;
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

    BufferLayout BufferLayout::merge(const BufferLayout& superset, const BufferLayout& subset)
    {
        if (superset.arrayDepth() > 1 || subset.arrayDepth() > 1 ||
            superset.offset()!= superset.absoluteOffset() || subset.offset()!= subset.absoluteOffset() ||
            superset.offset()!=0 || subset.offset()!=0 ||
            superset.type() != GraphicsType::STRUCT || subset.type() != GraphicsType::STRUCT)
        {
            throw std::runtime_error("BufferDescriptorLayouts can only be merged if both are top level struct types");
        }
        std::vector<BufferLayout> newSets;
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

            if (super.type()!= sub.type())
            {
                throw std::runtime_error("Layouts are incompatible");
            }
            if (super.type() == GraphicsType::STRUCT)
            {
                if (compatible(super,sub) == 0)
                {
                    throw std::runtime_error("Layouts are incompatible");
                }
            }

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

        return BufferLayout(superset.name(),GraphicsType::STRUCT,1,std::move(newSets),maxSize,0,0);


    }

    void BufferLayout::move(BufferLayout& from)
    {
        _name.swap(from._name);
        _type= from._type;
        _arrayDepth = from._arrayDepth;
        _children.swap(from._children);
        _size = from._size;
        _offset=from._offset;
        _absoluteOffset=from._absoluteOffset;
    }

    void BufferLayout::copy(const BufferLayout& from)
    {
        _name = from._name;
        _type= from._type;
        _arrayDepth = from._arrayDepth;
        _children = from._children;
        _size = from._size;
        _offset=from._offset;
        _absoluteOffset=from._absoluteOffset;
    }

    bool BufferLayout::compatibleRecursive(const BufferLayout& a,const BufferLayout& b)
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

    bool BufferLayout::proceeds(const BufferLayout& a, const BufferLayout& b)
    {
        if (a.offset()+a.size()<=b.offset())
        {
            return true;
        }
        return false;
    }

    bool BufferLayout::encompasses(const BufferLayout& a, const BufferLayout& b)
    {
        if (a.offset() <= b.offset() && a.offset()+a.size() >= b.offset()+b.size())
        {
            return true;
        }
        return false;
    }

    TexelBufferDescription::TexelBufferDescription(Pixels::Format format)
    {
        _format = format;
    }

    Pixels::Format TexelBufferDescription::format() const
    {
        return _format;
    }
} // slag
