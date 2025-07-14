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

    Descriptor::Descriptor(const std::string& name, DescriptorType type, uint32_t arrayDepth, uint32_t binding, ShaderStage::Flags visibleStages):_shape(type,arrayDepth,binding,visibleStages)
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
} // slag
