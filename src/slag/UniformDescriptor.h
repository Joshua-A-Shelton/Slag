#ifndef SLAG_UNIFORMDESCRIPTOR_H
#define SLAG_UNIFORMDESCRIPTOR_H

#include "GPUTypes.h"
#include <string>

namespace slag
{
    class UniformDescriptor
    {
    public:
        UniformDescriptor(uint32_t binding,uint32_t offset, uint32_t size, GraphicsTypes::GraphicsType type, std::string name);
        uint32_t binding()const;
        uint32_t offset()const;
        uint32_t size()const;
        GraphicsTypes::GraphicsType type()const;
        const std::string& name()const;
        static bool compareBinding(UniformDescriptor& descriptor1, UniformDescriptor descriptor2);
    private:
        uint32_t _binding=0;
        uint32_t _offset=0;
        uint32_t _size=0;
        GraphicsTypes::GraphicsType _type;
        std::string _name;
    };
}
#endif //SLAG_UNIFORMDESCRIPTOR_H
