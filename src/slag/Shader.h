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

#include <filesystem>
#include "VertexDescription.h"
#include "FrameBufferDescription.h"
#include "ShaderProperties.h"

namespace slag
{
    ///Set of stages that occurs in shader execution
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
    ///Small amount of data that is saved to the command buffer itself for shaders to use
    struct PushConstantRange
    {
        ShaderStages stageFlags;
        uint32_t offset;
        uint32_t size;
    };

    class DescriptorGroup;
    ///Represents a stage of shader execution in SPIR-V
    class ShaderModule
    {
    public:
        ShaderModule(ShaderStages stage, void* data, size_t size);
        ShaderModule(ShaderStages stage, std::filesystem::path path);
        ShaderModule(const ShaderModule&)=delete;
        ShaderModule& operator=(const ShaderModule&)=delete;
        ShaderModule(ShaderModule&& from);
        ShaderModule& operator=(ShaderModule&& from);
        ///Raw bytes of the shader code
        void* data();
        ///Size of the shader code data
        size_t dataSize();
        ///Which stage of execution this module represents
        ShaderStages stage();
    private:
        void move(ShaderModule&& from);
        ShaderStages _stage;
        std::vector<char> _shaderData;
    };

    ///Program run on the GPU run highly in parallel
    class Shader
    {
    public:
        virtual ~Shader()=default;
        ///Number of descriptor groups this shader has
        virtual size_t descriptorGroupCount()=0;
        ///Retrieve descriptor group at index
        virtual DescriptorGroup* descriptorGroup(size_t index)=0;
        ///Retrieve descriptor group at index
        virtual DescriptorGroup* operator[](size_t index)=0;
        ///Number of push constant ranges
        virtual size_t pushConstantRangeCount()=0;
        ///Retrieve push constant at index
        virtual PushConstantRange pushConstantRange(size_t index)=0;
        /**
         * Create a new shader
         * @param modules Array of stages of shader execution
         * @param moduleCount Number of objects in modules array
         * @param descriptorGroups Array of descriptor groups this shader contains, nullptr to get via reflection
         * @param descriptorGroupCount Number of descriptor groupps in descriptorGroups array
         * @param properties Specific details that determine a number of shader execution properties
         * @param vertexDescription Definition of the vertex structure this shader will output to, nullptr to get via reflection
         * @param frameBufferDescription Description of the Render targets this shader will render to
         * @return
         */
        static Shader* newShader(ShaderModule* modules, size_t moduleCount, DescriptorGroup** descriptorGroups, size_t descriptorGroupCount, ShaderProperties& properties, VertexDescription* vertexDescription, FrameBufferDescription& frameBufferDescription);
    };

} // slag

#endif //SLAG_SHADER_H
