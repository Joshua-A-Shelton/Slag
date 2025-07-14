#ifndef SLAG_SHADERPIPELINE_H
#define SLAG_SHADERPIPELINE_H

#include <cstdint>
#include <vector>

#define SHADER_STAGE_DEFINTITIONS(DEFINITION) \
DEFINITION(VERTEX,0b0000000000000001,VK_SHADER_STAGE_VERTEX_BIT,D3D12_SHVER_VERTEX_SHADER) \
DEFINITION(GEOMETRY,0b0000000000000010,VK_SHADER_STAGE_GEOMETRY_BIT,D3D12_SHVER_GEOMETRY_SHADER) \
DEFINITION(FRAGMENT,0b0000000000000100,VK_SHADER_STAGE_FRAGMENT_BIT,D3D12_SHVER_PIXEL_SHADER) \
DEFINITION(COMPUTE,0b0000000000001000,VK_SHADER_STAGE_COMPUTE_BIT,D3D12_SHVER_COMPUTE_SHADER) \
DEFINITION(RAY_GENERATION,0b0000000000010000,VK_SHADER_STAGE_RAYGEN_BIT_KHR,D3D12_SHVER_RAY_GENERATION_SHADER) \
DEFINITION(ANY_HIT,0b0000000000100000,VK_SHADER_STAGE_ANY_HIT_BIT_KHR,D3D12_SHVER_ANY_HIT_SHADER) \
DEFINITION(CLOSEST_HIT,0b0000000001000000,VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,D3D12_SHVER_CLOSEST_HIT_SHADER) \
DEFINITION(MISS,0b0000000010000000,VK_SHADER_STAGE_MISS_BIT_KHR,D3D12_SHVER_MISS_SHADER) \
DEFINITION(INTERSECTION,0b0000000100000000,VK_SHADER_STAGE_INTERSECTION_BIT_KHR,D3D12_SHVER_INTERSECTION_SHADER) \
DEFINITION(CALLABLE,0b0000001000000000,VK_SHADER_STAGE_CALLABLE_BIT_KHR,D3D12_SHVER_CALLABLE_SHADER) \
DEFINITION(MESH,0b0000010000000000,VK_SHADER_STAGE_MESH_BIT_EXT,D3D12_SHVER_MESH_SHADER)   \
DEFINITION(TASK,0b0000100000000000,VK_SHADER_STAGE_TASK_BIT_EXT,D3D12_SHVER_AMPLIFICATION_SHADER) \


namespace slag
{
    class UniformBufferDescriptorLayout;
    class DescriptorGroup;

    class ShaderStage
    {
    public:
        enum Flags: uint16_t
        {
#define DEFINITION(SlagName, SlagValue, VulkanName, DXName) SlagName = SlagValue,
            SHADER_STAGE_DEFINTITIONS(DEFINITION)
#undef DEFINITION
        };
    };
    ///Represents a stage of shader execution
    class ShaderCode
    {
    public:
        enum CodeLanguage
        {
            SPIRV,
            CUSTOM
        };
        ShaderCode(ShaderStage::Flags stage, void* data, size_t dataLength);
        ShaderCode(const ShaderCode&)=delete;
        ShaderCode& operator=(const ShaderCode&)=delete;
        ShaderCode(ShaderCode&& from);
        ShaderCode& operator=(ShaderCode&& from);
        ///Raw bytes of shader code
        void* data();
        ///Size in bytes of shader code
        size_t dataSize();
        ///Stage of shader pipeline this code represents
        ShaderStage::Flags stage();
        CodeLanguage language();
    private:
        void move(ShaderCode& from);
        ShaderStage::Flags _stage;
        CodeLanguage _codeLanguage;
        std::vector<unsigned char> _data;
    };

    class ShaderPipeline
    {
    public:
        virtual ~ShaderPipeline()=default;
        ///Number of descriptor groups this shader has
        virtual uint32_t descriptorGroupCount()=0;
        ///Retrieve descriptor group at index
        virtual DescriptorGroup* descriptorGroup(size_t index)=0;
        ///Retrieve descriptor group at index
        virtual DescriptorGroup* operator[](size_t index)=0;
        /**
         * Retrieve the layout of a uniform buffer descriptor
         * @param descriptorGroup the descriptor group index
         * @param descriptorBinding the binding of the uniform buffer
         * @return Layout of a uniform buffer descriptor, or null if the index isn't a uniform buffer
         */
        virtual UniformBufferDescriptorLayout* uniformBufferLayout(uint32_t descriptorGroup,uint32_t descriptorBinding)=0;

        static std::vector<ShaderCode::CodeLanguage> acceptedLanguages();

    };
} // slag

#endif //SLAG_SHADERPIPELINE_H
