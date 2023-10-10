#include "UniformDescriptor.h"
namespace slag
{

    UniformDescriptor::UniformDescriptor(uint32_t binding, uint32_t offset, uint32_t size, GraphicsTypes::GraphicsType type, std::string name)
    {
        _binding = binding;
        _offset = offset;
        _size = size;
        _type = type;
        _name = name;
    }

    uint32_t UniformDescriptor::binding() const
    {
        return _binding;
    }

    uint32_t UniformDescriptor::offset() const
    {
        return _offset;
    }

    uint32_t UniformDescriptor::size() const
    {
        return _size;
    }

    GraphicsTypes::GraphicsType UniformDescriptor::type() const
    {
        return _type;
    }

    const std::string &UniformDescriptor::name() const
    {
        return _name;
    }

}