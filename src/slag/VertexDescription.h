#ifndef CRUCIBLEEDITOR_VERTEXDESCRIPTION_H
#define CRUCIBLEEDITOR_VERTEXDESCRIPTION_H

#include <cstdint>
#include <vector>
#include "PixelFormat.h"

namespace slag
{

    struct VertexAttribute
    {
        uint32_t location;
        Pixels::PixelFormat storageType;
        uint32_t offset;
    };

    class VertexDescription
    {
    public:
        VertexDescription(std::vector<VertexAttribute>& attributes);
        std::vector<VertexAttribute>& attributes();
    private:
        std::vector<VertexAttribute> _attributes;
    };

    class VertexDescriptionBuilder
    {
    public:
        VertexDescriptionBuilder& add(Pixels::PixelFormat backingStorageType);
        VertexDescription build();
    private:
        std::vector<VertexAttribute> _attributes;
        uint32_t _location = 0;
        uint32_t _offset = 0;
    };

} // slag

#endif //CRUCIBLEEDITOR_VERTEXDESCRIPTION_H
