#include "VertexDescription.h"

namespace slag
{
    VertexDescription::VertexDescription(std::vector<VertexAttribute> &attributes)
    {
        _attributes = attributes;
    }

    std::vector<VertexAttribute> &VertexDescription::attributes()
    {
        return _attributes;
    }

    VertexDescriptionBuilder &VertexDescriptionBuilder::add(Pixels::PixelFormat backingStorageType)
    {
        _attributes.push_back({.location = _location, .storageType = backingStorageType, .offset = _offset});
        _location++;
        _offset+= Pixels::pixelBytes(backingStorageType);
        return *this;
    }

    VertexDescription VertexDescriptionBuilder::build()
    {
        return VertexDescription(_attributes);
    }
} // slag