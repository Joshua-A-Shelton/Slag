#include "VertexDescription.h"

#include "ShaderCode.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    VertexAttribute::VertexAttribute(const std::string& semanticName, GraphicsType dataType, uint32_t offset, uint32_t arrayLength)
    {
        SLAG_ASSERT(dataType != GraphicsType::STRUCT && "dataType cannot be struct for vertex attribute");
        SLAG_ASSERT(((GraphicsTypeBits)dataType & GraphicsTypeBits::ANY_MATRIX_BIT) == GraphicsTypeBits::NONE_BIT && "matrices must be decomposed into arrays");
        _semanticName = semanticName;
        _dataType = dataType;
        _offset = offset;
        _arrayLength = arrayLength;
    }


    const std::string& VertexAttribute::semanticName() const
    {
        return _semanticName;
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

    bool operator==(const VertexAttribute& a, const VertexAttribute& b)
    {
        if (a._semanticName == b._semanticName && a.dataType() == b.dataType() && a.offset() == b.offset() && a.arrayLength() == b.arrayLength())
        {
            return true;
        }
        return false;
    }

    bool operator!=(const VertexAttribute& a, const VertexAttribute& b)
    {
        return !(a == b);
    }

    VertexDescription::VertexDescription(uint32_t attributeChannels)
    {
        _attributes.resize(attributeChannels);
    }

    VertexDescription& VertexDescription::add(const VertexAttribute& attribute, uint32_t attributeChannel)
    {
        if (!_attributes.at(attributeChannel).empty())
        {
            auto& lastAttribute = _attributes.at(attributeChannel).back();
            if (attribute.offset() <= lastAttribute.offset())
            {
                throw std::runtime_error("Attribute must have a greater offset than previous attribute in stream");
            }
        }
        auto index = _attributeMap.find(attribute.semanticName());
        if(index != _attributeMap.end())
        {
            throw std::runtime_error("Attribute with semantic name already exists for this vertex description");
        }
        _attributeMap.insert(std::pair(attribute.semanticName(),AttributeIndex{attributeChannel,(uint32_t)_attributes.at(attributeChannel).size()}));
        _attributes.at(attributeChannel).push_back(attribute);
        return *this;
    }

    VertexDescription& VertexDescription::add(const std::string& semanticName, GraphicsType dataType, uint32_t offset, uint32_t arrayLength, size_t attributeChannel)
    {

        add(VertexAttribute(semanticName,dataType,offset,arrayLength),attributeChannel);
        return *this;
    }

    uint32_t VertexDescription::attributeCount() const
    {
        size_t count = 0;
        for(auto& channel: _attributes)
        {
            count += channel.size();
        }
        return count;
    }

    uint32_t VertexDescription::attributeCount(size_t channel) const
    {
        return _attributes.at(channel).size();
    }

    uint32_t VertexDescription::attributeChannels() const
    {
        return _attributes.size();
    }

    VertexAttribute& VertexDescription::attribute(size_t channel,size_t index)
    {
        return _attributes.at(channel).at(index);
    }

    VertexAttribute* VertexDescription::attribute(const std::string& semanticName)
    {
        auto index = _attributeMap.find(semanticName);
        if (index == _attributeMap.end())
        {
            return nullptr;
        }
        return  &_attributes.at(index->second.attributeChannel).at(index->second.attributeIndex);
    }

    bool operator==(const VertexDescription& a, const VertexDescription& b)
    {
        if (a.attributeChannels() == b.attributeChannels())
        {
            for (int i=0; i<a.attributeChannels(); i++)
            {
                if (a._attributes.at(i).size() != b._attributes.at(i).size())
                {
                    return false;
                }
                for (int j=0; j<a._attributes.at(i).size(); j++)
                {
                    if (a._attributes.at(i).at(j) != b._attributes.at(i).at(j))
                    {
                        return false;
                    }
                }
            }
            return true;
        }
        return false;
    }

    bool operator!=(const VertexDescription& a, const VertexDescription& b)
    {
        return !(a == b);
    }
} // slag