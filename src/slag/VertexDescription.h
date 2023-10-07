#ifndef SLAG_VERTEXDESCRIPTION_H
#define SLAG_VERTEXDESCRIPTION_H
#include "GPUTypes.h"
#include <vector>
namespace slag
{

    struct VertexAttribute
    {
        VertexAttributes::VertexAttributeType format;
        uint32_t count;
    };
    class VertexDescription
    {
    public:
        VertexDescription()=default;
        VertexDescription(const VertexDescription& from);
        VertexDescription& operator=(const VertexDescription& from);
        VertexDescription(VertexDescription&& from);
        VertexDescription& operator=(VertexDescription&& from);
        VertexDescription& addAttribute(VertexAttribute type);
        size_t attributeCount();
        VertexAttribute attribute(size_t index);
        VertexAttribute operator[](size_t index);
    private:
        std::vector<VertexAttribute> _types;
        void copy(const VertexDescription& from);
        void move(VertexDescription&& from);
    };

} // slag

#endif //SLAG_VERTEXDESCRIPTION_H
