#ifndef SLAG_UNIFORMBUFFERDESCRIPTORLAYOUT_H
#define SLAG_UNIFORMBUFFERDESCRIPTORLAYOUT_H
#include <vector>
#include <string>
#include <memory>
#include "GraphicsTypes.h"
namespace slag
{

    class UniformBufferDescriptorLayout
    {
    public:
        /**
         * Create a UniformBufferDescriptorLayout, will assume all parameters are correct without checking. This will allow manual building, but requires the user to keep track of the details when creating the children's offset and sizes
         * @param name
         * @param type
         * @param arrayDepth
         * @param children
         * @param size
         * @param offset
         */
        UniformBufferDescriptorLayout(const std::string& name, GraphicsTypes::GraphicsType type, uint32_t arrayDepth, std::vector<UniformBufferDescriptorLayout>&& children, size_t size, size_t offset);
        UniformBufferDescriptorLayout()=delete;
        UniformBufferDescriptorLayout(const UniformBufferDescriptorLayout&)=delete;
        UniformBufferDescriptorLayout& operator=(const UniformBufferDescriptorLayout&)=delete;
        UniformBufferDescriptorLayout(UniformBufferDescriptorLayout&& from);
        UniformBufferDescriptorLayout& operator=(UniformBufferDescriptorLayout&& from);
        ///The descriptive name of the object in the buffer, may be empty string
        const std::string& name()const;
        ///The type this layout represents
        GraphicsTypes::GraphicsType type()const;
        ///How many children layouts this layout has. Zero for non-struct layouts
        size_t childrenCount()const;
        ///Size in bytes this layout takes in the memory buffer
        size_t size()const;
        ///the offset position in bytes from the root of the buffer
        size_t offset()const;
        const UniformBufferDescriptorLayout& operator[](size_t index)const;
    private:
        void move(UniformBufferDescriptorLayout&& from);
        std::string _name;
        GraphicsTypes::GraphicsType _type= GraphicsTypes::STRUCT;
        uint32_t _arrayDepth = 1;
        std::vector<UniformBufferDescriptorLayout> _children;
        size_t _size = 0;
        size_t _offset=0;

    };

} // slag

#endif //SLAG_UNIFORMBUFFERDESCRIPTORLAYOUT_H