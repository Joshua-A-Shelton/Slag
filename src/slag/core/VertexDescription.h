#ifndef SLAG_VERTEXDESCRIPTION_H
#define SLAG_VERTEXDESCRIPTION_H

#include <unordered_map>

#include "GraphicsTypes.h"
#include <vector>

namespace slag
{
    class ShaderMetaData;
}

namespace slag
{
    class VertexInputAttribute;

    ///Describes member of vertex
    class VertexAttribute
    {
    public:
        const std::string& semanticName()const;
        ///What data type this member of the vertex is
        GraphicsType dataType()const;
        ///The offset in bytes from the start of the buffer this member is
        uint32_t offset()const;
        uint32_t arrayLength()const;
        /**
         * Create a vertex attribute
         * @param semanticName Name of this attribute to match the name of the attribute in the shader
         * @param dataType What data type this member of the vertex is
         * @param offset The offset in bytes from the start of the vertex this member is
         * @param arrayLength Number of elements of this type in an array
         */
        VertexAttribute(const std::string& semanticName, GraphicsType dataType, uint32_t offset, uint32_t arrayLength=1);

        friend bool operator==(const VertexAttribute& a, const VertexAttribute& b);
        friend bool operator!=(const VertexAttribute& a, const VertexAttribute& b);

    private:
        std::string _semanticName;
        GraphicsType _dataType = GraphicsType::UNKNOWN;
        uint32_t _offset = 0;
        uint32_t _arrayLength = 0;


    };
    ///Describes how a vertex will be read from a shader
    class VertexDescription
    {
    public:
        /**
         * Create a new description of a vertex
         * @param attributeChannels Number of different buffers vertex data can be sourced from
         */
        VertexDescription(uint32_t attributeChannels);
        /**
         * Add a new attribute to the description of a vertex
         * @param attribute Data member of a vertex
         * @param attributeChannel Index of the buffer the attribute will be in
         * @return
         */
        VertexDescription& add(const VertexAttribute& attribute, uint32_t attributeChannel);
        /**
         * Add a new attribute to the description of a vertex
         * @param semanticName Name of this attribute to match the name of the attribute in the shader
         * @param dataType What data type this member of the vertex is
         * @param offset The offset in bytes from the start of the vertex this member is
         * @param arrayLength Number of elements of this type in an array
         * @param attributeChannel Index of the buffer the attribute will be in
         * @return
         */
        VertexDescription& add(const std::string& semanticName, GraphicsType dataType, uint32_t offset, uint32_t arrayLength, size_t attributeChannel);
        ///Number of attributes across all channels
        uint32_t attributeCount() const;
        /**
         * Number of attributes in a given channel
         * @param channel Index of the channel
         * @return
         */
        uint32_t attributeCount(size_t channel) const;
        ///Number of different buffers vertex data can be sourced from
        uint32_t attributeChannels() const;
        /**
         * Retrieve the attribute in a given channel
         * @param channel The channel to retrieve from
         * @param index The index of the attribute to retrieve
         * @return
         */
        VertexAttribute& attribute(size_t channel, size_t index);

        VertexAttribute* attribute(const std::string& semanticName);

        friend bool operator==(const VertexDescription& a, const VertexDescription& b);
        friend bool operator!=(const VertexDescription& a, const VertexDescription& b);
    private:
        struct AttributeIndex
        {
            uint32_t attributeChannel;
            uint32_t attributeIndex;
        };
        std::vector<std::vector<VertexAttribute>> _attributes;
        std::unordered_map<std::string,AttributeIndex> _attributeMap;
    };

} // slag

#endif //SLAG_VERTEXDESCRIPTION_H
