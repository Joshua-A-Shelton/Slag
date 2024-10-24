#include "Descriptor.h"

namespace slag
{

    Descriptor::Descriptor(const std::string& name, DescriptorType type, uint32_t arrayDepth, uint32_t binding, ShaderStages visibleStages): _shape(type,arrayDepth,binding,visibleStages)
    {
        _name = name;
    }

    Descriptor::Descriptor(Descriptor&& from): _shape(from._shape)
    {
        move(from);
    }

    Descriptor& Descriptor::operator=(Descriptor&& from)
    {
        move(from);
        return *this;
    }

    void Descriptor::move(Descriptor& uniform)
    {
        _name.swap(uniform._name);
        _shape = uniform._shape;
    }

    Descriptor::Descriptor(Descriptor& from): _shape(from._shape)
    {
        copy(from);
    }

    Descriptor& Descriptor::operator=(Descriptor& from)
    {
        copy(from);
        return *this;
    }


    void Descriptor::copy(Descriptor& from)
    {
        _name = from._name;
        _shape = from._shape;
    }

    const std::string& Descriptor::name()const
    {
        return _name;
    }

    const Descriptor::Shape& Descriptor::shape() const
    {
        return _shape;
    }


    bool Descriptor::Shape::operator==(const Descriptor::Shape& to)const
    {
        return binding == to.binding && type == to.type && arrayDepth == to.arrayDepth && visibleStages == to.visibleStages;
    }

    bool Descriptor::Shape::operator!=(const Descriptor::Shape& to)const
    {
        return !(*this == to);
    }
} // slag