#ifndef SLAG_VERTEXDESCRIPTION_H
#define SLAG_VERTEXDESCRIPTION_H
#include <string>
#include <vector>
#include <slag/core/GraphicsTypes.h>
namespace slag
{
    ///Describes an input into a vertex shader
    class VertexAttribute
    {
    public:
        /**
         *
         * @param name Name to match on when making a shader pipeline
         * @param loadAs Format to load the data from the buffer as
         * @param offset Offset into the vertex binding buffer this attribute is
         */
        VertexAttribute(const std::string& name, PixelFormat loadAs, uint32_t offset);
        ///Name to match on when making a shader pipeline
        [[nodiscard]] std::string name()const;
        ///Graphics Type of the vertex attribute
        [[nodiscard]] PixelFormat loadAs()const;
        ///Offset into the vertex binding buffer this attribute is
        [[nodiscard]] uint32_t offset()const;
    private:
        std::string _name;
        PixelFormat _loadAs = PixelFormat::UNDEFINED;
        uint32_t _offset = 0u;
    };
    ///Represents a buffer layout that is bound for inputting data into vertex shaders at a given index
    class VertexBinding
    {
    public:
        /**
         *
         * @param index Binding index
         * @param stride Size of each "vertex" in this binding
         * @param attributes Attributes this binding contains
         */
        VertexBinding(uint32_t index, uint32_t stride, const std::vector<VertexAttribute>& attributes);
        /**
         *
         * @param index Binding index
         * @param attributes Attributes this binding contains
         */
        VertexBinding(uint32_t index, std::vector<VertexAttribute>&& attributes);
        ///The binding index
        [[nodiscard]] uint32_t bindingIndex()const;
        ///Size of each "vertex" in this binding
        [[nodiscard]] uint32_t stride()const;
        ///Number of attributes this binding holds
        [[nodiscard]] uint32_t attributeCount()const;
        ///Get an attribute at a given index
        [[nodiscard]] const VertexAttribute& operator[](uint32_t index)const;
    private:
        uint32_t _index = 0u;
        uint32_t _stride = 0u;
        std::vector<VertexAttribute> _attributes;
    };
    ///Describes how a vertex is input into vertex shaders
    class VertexDescription
    {
    public:
        VertexDescription(std::vector<VertexBinding> bindings);
        VertexDescription(std::vector<VertexBinding>&& bindings);
        ///Number of vertex bindings
        [[nodiscard]] uint32_t bindingCount()const;
        ///Retrieve binding at an index (not the *binding* index of the given vertex binding)
        const VertexBinding& operator[](uint32_t index)const;
        uint32_t attributeCount()const;
    private:
        std::vector<VertexBinding> _bindings;
    };
} // slag

#endif //SLAG_VERTEXDESCRIPTION_H