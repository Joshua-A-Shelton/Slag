#include "VertexDescription.h"

#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    VertexAttribute::VertexAttribute(const std::string& name, PixelFormat loadAs, uint32_t offset)
    {
        _name = name;
        _loadAs = loadAs;
        _offset = offset;
    }

    const std::string& VertexAttribute::name() const
    {
        return _name;
    }

    PixelFormat VertexAttribute::loadAs() const
    {
        return _loadAs;
    }

    uint32_t VertexAttribute::offset() const
    {
        return _offset;
    }

    VertexBinding::VertexBinding(uint32_t index, uint32_t stride, const std::vector<VertexAttribute>& attributes)
    {
        _index = index;
        _stride = stride;
        _attributes = attributes;
    }

    VertexBinding::VertexBinding(uint32_t index, std::vector<VertexAttribute>&& attributes)
    {
        _index = index;
        _attributes = std::move(attributes);
    }

    uint32_t VertexBinding::bindingIndex() const
    {
        return _index;
    }

    uint32_t VertexBinding::stride() const
    {
        return _stride;
    }

    uint32_t VertexBinding::attributeCount() const
    {
        return _attributes.size();
    }

    const VertexAttribute& VertexBinding::operator[](uint32_t index) const
    {
        SLAG_ASSERT(index < _attributes.size() && "Index out of bounds");
        return _attributes[index];
    }

    VertexDescription::VertexDescription(std::vector<VertexBinding> bindings)
    {
        _bindings = bindings;
    }

    VertexDescription::VertexDescription(std::vector<VertexBinding>&& bindings)
    {
        _bindings = std::move(bindings);
    }

    uint32_t VertexDescription::bindingCount() const
    {
        return _bindings.size();
    }

    const VertexBinding& VertexDescription::operator[](uint32_t index) const
    {
        SLAG_ASSERT(index < _bindings.size() && "Index out of bounds");
        return _bindings[index];
    }

    uint32_t VertexDescription::attributeCount() const
    {
        uint32_t attributes = 0;
        for (auto& binding : _bindings)
        {
            attributes += binding.attributeCount();
        }
        return attributes;
    }
} // slag