#include "VertexDescription.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    VertexAttribute::VertexAttribute(GraphicsType dataType, uint32_t offset, uint32_t arrayLength)
    {
        SLAG_ASSERT(dataType != GraphicsType::STRUCT && "dataType cannot be struct for vertex attribute");
        SLAG_ASSERT(((GraphicsTypeBits)dataType & GraphicsTypeBits::ANY_MATRIX_BIT) == GraphicsTypeBits::NONE_BIT && "matrices must be decomposed into arrays");
        _dataType = dataType;
        _offset = offset;
        _arrayLength = arrayLength;
    }


    GraphicsType VertexAttribute::dataType() const
    {
        return _dataType;
    }

    uint32_t VertexAttribute::offset() const
    {
        return _offset;
    }

    uint32_t VertexAttribute::arrayLength() const
    {
        return _arrayLength;
    }

    VertexDescription::VertexDescription(size_t attributeChannels)
    {
        _attributes.resize(attributeChannels);
    }

    VertexDescription& VertexDescription::add(const VertexAttribute& attribute, size_t attributeChannel)
    {
        if (!_attributes.at(attributeChannel).empty())
        {
            auto& lastAttribute = _attributes.at(attributeChannel).back();
            if (attribute.offset() <= lastAttribute.offset())
            {
                throw std::runtime_error("Attribute must have a greater offset than previous attribute in stream");
            }
        }
        _attributes.at(attributeChannel).push_back(attribute);
        return *this;
    }

    VertexDescription& VertexDescription::add(GraphicsType dataType, uint32_t offset, size_t attributeChannel, uint32_t arrayLength)
    {
        _attributes.at(attributeChannel).emplace_back(dataType,offset,arrayLength);
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