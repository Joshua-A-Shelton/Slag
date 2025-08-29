#ifndef SLAG_VERTEXDESCRIPTION_H
#define SLAG_VERTEXDESCRIPTION_H

#include "GraphicsTypes.h"
#include <vector>

namespace slag
{
    ///Describes member of vertex
    class VertexAttribute
    {
    public:
        ///What data type this member of the vertex is
        GraphicsType dataType()const;
        ///The offset in bytes from the start of the buffer this member is
        uint32_t offset()const;
        /**
         * Create a vertex attribute
         * @param dataType What data type this member of the vertex is
         * @param offset The offset in bytes from the start of the vertex this member is
         */
        VertexAttribute(GraphicsType dataType, uint32_t offset);

    private:
        GraphicsType _dataType = GraphicsType::STRUCT;
        uint32_t _offset = 0;

    };
    ///Describes how a vertex will be read from a shader
    class VertexDescription
    {
    public:
        /**
         * Create a new description of a vertex
         * @param attributeChannels Number of different buffers vertex data can be sourced from
         */
        VertexDescription(size_t attributeChannels);
        /**
         * Add a new attribute to the description of a vertex
         * @param attribute Data member of a vertex
         * @param attributeChannel Index of the buffer the attribute will be in
         * @return
         */
        VertexDescription& add(const VertexAttribute& attribute, size_t attributeChannel);
        /**
         * Add a new attribute to the description of a vertex
         * @param dataType What data type this member of the vertex is
         * @param offset The offset in bytes from the start of the vertex this member is
         * @param attributeChannel Index of the buffer the attribute will be in
         * @return
         */
        VertexDescription& add(GraphicsType dataType, uint32_t offset, size_t attributeChannel);
        ///Number of attributes across all channels
        size_t attributeCount()const;
        /**
         * Number of attributes in a given channel
         * @param channel Index of the channel
         * @return
         */
        size_t attributeCount(size_t channel)const;
        ///Number of different buffers vertex data can be sourced from
        size_t attributeChannels()const;
        /**
         * Retrieve the attribute in a given channel
         * @param channel The channel to retrieve from
         * @param index The index of the attribute to retrieve
         * @return
         */
        VertexAttribute& attribute(size_t channel, size_t index);
    private:
        std::vector<std::vector<VertexAttribute>> _attributes;
    };

} // slag

#endif //SLAG_VERTEXDESCRIPTION_H
