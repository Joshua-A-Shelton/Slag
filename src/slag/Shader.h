#ifndef SLAG_SHADER_H
#define SLAG_SHADER_H

#define SHADER_STAGE_DEFINTITIONS(DEFINITION) \
DEFINITION(VERTEX,VK_SHADER_STAGE_VERTEX_BIT,D3D12_SHVER_VERTEX_SHADER) \
DEFINITION(GEOMETRY,VK_SHADER_STAGE_GEOMETRY_BIT,D3D12_SHVER_GEOMETRY_SHADER) \
DEFINITION(FRAGMENT,VK_SHADER_STAGE_FRAGMENT_BIT,D3D12_SHVER_PIXEL_SHADER) \
DEFINITION(COMPUTE,VK_SHADER_STAGE_COMPUTE_BIT,D3D12_SHVER_COMPUTE_SHADER) \
DEFINITION(RAY_GENERATION,VK_SHADER_STAGE_RAYGEN_BIT_KHR,D3D12_SHVER_RAY_GENERATION_SHADER) \
DEFINITION(ANY_HIT,VK_SHADER_STAGE_ANY_HIT_BIT_KHR,D3D12_SHVER_ANY_HIT_SHADER) \
DEFINITION(CLOSEST_HIT,VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,D3D12_SHVER_CLOSEST_HIT_SHADER) \
DEFINITION(MISS,VK_SHADER_STAGE_MISS_BIT_KHR,D3D12_SHVER_MISS_SHADER) \
DEFINITION(INTERSECTION,VK_SHADER_STAGE_INTERSECTION_BIT_KHR,D3D12_SHVER_INTERSECTION_SHADER) \
DEFINITION(CALLABLE,VK_SHADER_STAGE_CALLABLE_BIT_KHR,D3D12_SHVER_CALLABLE_SHADER) \
DEFINITION(MESH,VK_SHADER_STAGE_MESH_BIT_EXT,D3D12_SHVER_MESH_SHADER)   \
DEFINITION(TASK,VK_SHADER_STAGE_TASK_BIT_EXT,D3D12_SHVER_AMPLIFICATION_SHADER) \

namespace slag
{
    class ShaderStages
    {
    private:
        int _value;
        explicit ShaderStages(int val){ _value=val;}
    public:
        friend class ShaderStageFlags;
        ShaderStages operator| (ShaderStages b) const
        {
            return ShaderStages(_value | b._value);
        }

        ShaderStages& operator |=(ShaderStages b)
        {
            _value = _value|b._value;
            return *this;
        }

        ShaderStages operator&(ShaderStages b) const
        {
            return ShaderStages(_value & b._value);
        }

        ShaderStages& operator&=(ShaderStages b)
        {
            _value = _value&b._value;
            return *this;
        }

        ShaderStages operator~() const
        {
            return ShaderStages(~_value);
        }

        bool operator == (const ShaderStages& to)const
        {
            return  _value == to._value;
        }

        bool operator != (const ShaderStages& to)const
        {
            return _value != to._value;
        }
    };

    class ShaderStageFlags
    {
#define DEFINITION(slagName, vulkanName, directXName) inline static ShaderStages _##slagName = ShaderStages(0);
        SHADER_STAGE_DEFINTITIONS(DEFINITION)
#undef DEFINITION
    public:
#define DEFINITION(slagName, vulkanName, directXName) inline static const ShaderStages& slagName = (_##slagName); /***Only use this if you *REALLY* know what you're doing, will override underlying library value for flag*/static void set##slagName##Value(int val){_##slagName._value = val;}
        SHADER_STAGE_DEFINTITIONS(DEFINITION)
#undef DEFINITION
    };

    class DescriptorGroup;
    class Shader
    {
    public:
        virtual ~Shader()=default;
        virtual size_t descriptorGroupCount()=0;
        virtual DescriptorGroup* descriptorGroup(size_t index)=0;
        virtual DescriptorGroup* operator[](size_t index)=0;
    };

} // slag

#endif //SLAG_SHADER_H
