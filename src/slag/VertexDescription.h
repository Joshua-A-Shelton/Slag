#ifndef SLAG_VERTEXDESCRIPTION_H
#define SLAG_VERTEXDESCRIPTION_H

#include "GraphicsTypes.h"
#include <vector>

namespace slag
{

    class VertexAttribute
    {
    public:
        GraphicsTypes::GraphicsType dataType()const;
        uint32_t offset()const;
        VertexAttribute(GraphicsTypes::GraphicsType dataType, uint32_t offset);

    private:
        GraphicsTypes::GraphicsType _dataType = GraphicsTypes::STRUCT;
        uint32_t _offset = 0;

    };
    class VertexDescription
    {
    public:
        VertexDescription(size_t attributeChannels);
        VertexDescription& add(const VertexAttribute& attribute, size_t attributeChannel);
        VertexDescription& add(GraphicsTypes::GraphicsType dataType, uint32_t offset, size_t attributeChannel);
        size_t attributeCount()const;
        size_t attributeCount(size_t channel)const;
        size_t attributeChannels()const;
        VertexAttribute& attribute(size_t channel, size_t index);
    private:
        std::vector<std::vector<VertexAttribute>> _attributes;
    };

} // slag

#endif //SLAG_VERTEXDESCRIPTION_H
