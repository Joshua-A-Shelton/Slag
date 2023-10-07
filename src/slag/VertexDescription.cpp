#include "VertexDescription.h"

namespace slag
{
    VertexDescription &VertexDescription::addAttribute(VertexAttribute type)
    {
        _types.push_back(type);
        return *this;
    }

    size_t VertexDescription::attributeCount()
    {
        return _types.size();
    }

    VertexAttribute VertexDescription::attribute(size_t index)
    {
        return _types[index];
    }

    VertexAttribute VertexDescription::operator[](size_t index)
    {
        return _types[index];
    }

    VertexDescription::VertexDescription(const VertexDescription &from)
    {
        copy(from);
    }

    VertexDescription &VertexDescription::operator=(const VertexDescription &from)
    {
        copy(from);
        return *this;
    }

    VertexDescription::VertexDescription(VertexDescription &&from)
    {
        move(std::move(from));
    }

    VertexDescription &VertexDescription::operator=(VertexDescription &&from)
    {
        move(std::move(from));
        return *this;
    }

    void VertexDescription::copy(const VertexDescription &from)
    {
        _types = from._types;
    }

    void VertexDescription::move(VertexDescription &&from)
    {
        _types.swap(from._types);
    }
} // slag