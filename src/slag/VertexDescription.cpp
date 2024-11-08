#include "VertexDescription.h"
#include <cassert>

namespace slag
{
    VertexAttribute::VertexAttribute(GraphicsTypes::GraphicsType dataType, uint32_t offset)
    {
        assert(dataType != GraphicsTypes::GraphicsType::STRUCT && "dataType cannot be struct for vertex attribute");
        _dataType = dataType;
        _offset = offset;
    }


    GraphicsTypes::GraphicsType VertexAttribute::dataType() const
    {
        return _dataType;
    }

    uint32_t VertexAttribute::offset() const
    {
        return _offset;
    }

    VertexDescription::VertexDescription(size_t attributeChannels)
    {
        _attributes.resize(attributeChannels);
    }

    VertexDescription& VertexDescription::add(const VertexAttribute& attribute, size_t attributeChannel)
    {
        _attributes.at(attributeChannel).push_back(attribute);
        return *this;
    }

    VertexDescription& VertexDescription::add(GraphicsTypes::GraphicsType dataType, uint32_t offset, size_t attributeChannel)
    {
        _attributes.at(attributeChannel).emplace_back(dataType,offset);
        return *this;
    }

    size_t VertexDescription::attributeCount() const
    {
        size_t count = 0;
        for(auto& channel: _attributes)
        {
            count += channel.size();
        }
        return count;
    }

    size_t VertexDescription::attributeCount(size_t channel) const
    {
        return _attributes.at(channel).size();
    }

    size_t VertexDescription::attributeChannels() const
    {
        return _attributes.size();
    }

    VertexAttribute& VertexDescription::attribute(size_t channel,size_t index)
    {
        return _attributes.at(channel).at(index);
    }
} // slag