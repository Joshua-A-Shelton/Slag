#ifndef SLAG_DESCRIPTOR_H
#define SLAG_DESCRIPTOR_H

#include <string>
#include <vector>
#include "Shader.h"

namespace slag
{

    class Descriptor
    {
    public:
        enum DescriptorType
        {
            SAMPLER,
            SAMPLED_TEXTURE,
            SAMPLER_AND_TEXTURE,
            STORAGE_TEXTURE,
            UNIFORM_TEXEL_BUFFER,
            STORAGE_TEXEL_BUFFER,
            UNIFORM_BUFFER,
            STORAGE_BUFFER,
            INPUT_ATTACHMENT
        };
        struct Shape
        {
            DescriptorType type=DescriptorType::UNIFORM_BUFFER;
            uint32_t arrayDepth=1;
            uint32_t binding=0;
            ShaderStages visibleStages = ShaderStageFlags::VERTEX;

            bool operator == (const Shape& to)const;
            bool operator != (const Shape& to)const;
        };
        Descriptor()=default;
        Descriptor(const std::string& name, DescriptorType type, uint32_t arrayDepth, uint32_t binding, ShaderStages visibleStages);
        ~Descriptor()=default;
        Descriptor(Descriptor& from);
        Descriptor& operator=(Descriptor& from);
        Descriptor(Descriptor&& from);
        Descriptor& operator=(Descriptor&& from);
        const std::string& name()const;
        const Shape& shape()const;
    protected:
        void copy(Descriptor& from);
        void move(Descriptor& uniform);
        std::string _name;
        Shape _shape;

    };

} // slag

#endif //SLAG_DESCRIPTOR_H
