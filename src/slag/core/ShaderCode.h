#ifndef SLAG_SHADERCODE_H
#define SLAG_SHADERCODE_H
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
DEFINITION(TASK,0b0000100000000000,VK_SHADER_STAGE_TASK_BIT_EXT,D3D12_SHVER_AMPLIFICATION_SHADER)
#include <cstdint>
#include <filesystem>
#include <vector>

#include "Descriptor.h"
#include "GraphicsTypes.h"

namespace slag
{
    enum class ShaderStageFlags: uint16_t
    {
#define DEFINITION(SlagName, SlagValue, VulkanName, DXName) SlagName = SlagValue,
        SHADER_STAGE_DEFINTITIONS(DEFINITION)
#undef DEFINITION
    };

    inline ShaderStageFlags operator|(ShaderStageFlags a, ShaderStageFlags b)
    {
        return static_cast<ShaderStageFlags>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
    }

    inline ShaderStageFlags operator&(ShaderStageFlags a, ShaderStageFlags b)
    {
        return static_cast<ShaderStageFlags>(static_cast<uint16_t>(a) & static_cast<uint16_t>(b));
    }

    inline ShaderStageFlags operator~(ShaderStageFlags a)
    {
        return static_cast<ShaderStageFlags>(~static_cast<uint16_t>(a));
    }

    inline ShaderStageFlags operator|=(ShaderStageFlags& a, ShaderStageFlags b)
    {
        a = a | b;
        return a;
    }

    inline ShaderStageFlags operator&=(ShaderStageFlags& a, ShaderStageFlags b)
    {
        a = a & b;
        return a;
    }

    class VertexInputAttribute
    {
    public:
        VertexInputAttribute()=default;
        VertexInputAttribute(const std::string& semanticName, GraphicsType type, uint32_t arrayLength, uint64_t inputID);
        ~VertexInputAttribute() = default;
        VertexInputAttribute(const VertexInputAttribute&)=default;
        VertexInputAttribute(VertexInputAttribute&&)=default;
        VertexInputAttribute& operator=(const VertexInputAttribute&)=default;
        VertexInputAttribute& operator=(VertexInputAttribute&&)=default;
        [[nodiscard]] const std::string& semanticName()const;
        [[nodiscard]] GraphicsType type()const;
        [[nodiscard]] uint32_t arrayLength()const;
        ///This is a shader code language dependent id. They use different mechanisms to determine binding, but each platform should support squishing it into this id. Same ID means it occupies the same input slot across shaders
        [[nodiscard]] uint64_t inputID()const;
    private:
        std::string _semanticName;
        GraphicsType _type = GraphicsType::UNKNOWN;
        uint32_t _arrayLength = 0;
        uint64_t _inputID = 0;
    };

    class DescriptorBinding
    {
    public:
        DescriptorBinding()=default;
        DescriptorBinding(Descriptor descriptor, uint64_t bindingId);
        const Descriptor& descriptor()const;
        ///This is a shader code language dependent id. They use different mechanisms to determine binding, but each platform should support squishing it into this id. Same ID means it occupies the same input slot across shaders
        [[nodiscard]] uint64_t bindingId()const;
    private:
        Descriptor _descriptor{};
        uint64_t _bindingId=0;
        void copy(DescriptorBinding& other);
    };

    class DescriptorBindingGroup
    {
    public:
        DescriptorBindingGroup()=default;
        DescriptorBindingGroup(DescriptorBinding* bindings, uint32_t bindingCount, uint32_t groupIndex);
        ~DescriptorBindingGroup()=default;
        DescriptorBindingGroup(const DescriptorBinding&)=delete;
        DescriptorBindingGroup& operator=(const DescriptorBindingGroup&)=delete;
        DescriptorBindingGroup(DescriptorBindingGroup&& from);
        DescriptorBindingGroup& operator=(DescriptorBindingGroup&& from);

        [[nodiscard]] uint32_t descriptorGroupIndex() const;
        [[nodiscard]] uint32_t bindingCount()const;
        [[nodiscard]] const DescriptorBinding& descriptorBinding(uint32_t index) const;
    private:
        void move(DescriptorBindingGroup& from);
        uint32_t _descriptorGroupIndex = UINT32_MAX;
        std::vector<DescriptorBinding> _bindings;
    };

    class BufferDescriptorBindingLayout
    {
    public:
        BufferDescriptorBindingLayout(uint32_t descriptorGroupIndex, uint32_t descriptorIndex, BufferLayout&& layout);
        [[nodiscard]] uint32_t descriptorGroupIndex()const;
        [[nodiscard]] uint32_t descriptorIndex()const;
        [[nodiscard]] const BufferLayout& bufferLayout()const;
    private:
        uint32_t _descriptorBindingGroupIndex = 0;
        uint32_t _descriptorIndex = 0;
        BufferLayout _bufferLayout;
    };

    class TexelBufferDescriptorBinding
    {
    public:
        TexelBufferDescriptorBinding(uint32_t descriptorGroupIndex, uint32_t descriptorIndex, TexelBufferDescription&& description);
        [[nodiscard]] uint32_t descriptorGroupIndex()const;
        [[nodiscard]] uint32_t descriptorIndex()const;
        [[nodiscard]] const TexelBufferDescription& bufferDescription()const;
    private:
        uint32_t _descriptorBindingGroupIndex = 0;
        uint32_t _descriptorIndex = 0;
        TexelBufferDescription _bufferDescription;
    };

    class ShaderCode;
    class ShaderMetaData
    {
    public:
        ShaderMetaData(
            ShaderStageFlags stage,
            std::vector<VertexInputAttribute>&& vertexInputs,
            std::vector<DescriptorBindingGroup>&& bindingGroups,
            std::vector<BufferDescriptorBindingLayout>&& uniformLayouts,
            std::vector<BufferDescriptorBindingLayout>&& storageLayouts,
            std::vector<TexelBufferDescriptorBinding>&& texelLayouts,
            BufferLayout pushConstantLayout,
            uint32_t xComputeThreads,
            uint32_t yComputeThreads,
            uint32_t zComputeThreads);
        ~ShaderMetaData()=default;
        ShaderMetaData(const ShaderMetaData&) = delete;
        ShaderMetaData& operator=(const ShaderMetaData&) = delete;
        ShaderMetaData(ShaderMetaData&& from);
        ShaderMetaData& operator=(ShaderMetaData&& from);
        [[nodiscard]] uint32_t inputCount() const;
        [[nodiscard]] const VertexInputAttribute& inputVariable(uint32_t index) const;
        [[nodiscard]] uint32_t descriptorBindingGroupCount() const;
        [[nodiscard]] const DescriptorBindingGroup& descriptorBindingGroup(uint32_t index)const;
        [[nodiscard]] uint32_t uniformBufferLayoutCount()const;
        [[nodiscard]] const BufferDescriptorBindingLayout& uniformBufferLayout(uint32_t index)const;
        [[nodiscard]] uint32_t storageBufferLayoutCount()const;
        [[nodiscard]] const BufferDescriptorBindingLayout& storageBufferLayout(uint32_t index)const;
        [[nodiscard]] uint32_t texelBufferDescriptionCount()const;
        [[nodiscard]] const TexelBufferDescriptorBinding& texelBufferDescription(uint32_t index)const;
        BufferLayout pushConstantLayout()const;

        const BufferDescriptorBindingLayout* uniformBufferLayout(uint32_t bindingGroup, uint32_t descriptorIndex) const;
        const BufferDescriptorBindingLayout* storageBufferLayout(uint32_t bindingGroup, uint32_t descriptorIndex)const;
        const TexelBufferDescriptorBinding* texelBufferDescription(uint32_t bindingGroup, uint32_t descriptorIndex)const;

        [[nodiscard]] uint32_t xComputeThreads() const;
        [[nodiscard]] uint32_t yComputeThreads() const;
        [[nodiscard]] uint32_t zComputeThreads() const;
        [[nodiscard]] ShaderStageFlags stage()const;
        friend class ShaderCode;
        ///Can be set to get shader reflection data on custom code languages
        inline static ShaderMetaData (*customReflection)(ShaderCode& shaderCode)=nullptr;
    private:
        void move(ShaderMetaData& from);
        explicit ShaderMetaData(ShaderCode& shaderCode);
        ShaderStageFlags _stage = static_cast<ShaderStageFlags>(0);
        std::vector<VertexInputAttribute> _vertexInputs;
        std::vector<DescriptorBindingGroup> _bindingGroups;
        std::vector<BufferDescriptorBindingLayout> _uniformBufferLayouts;
        std::vector<BufferDescriptorBindingLayout> _storageBufferLayouts;
        std::vector<TexelBufferDescriptorBinding> _texelBufferDescriptions;
        BufferLayout _pushConstantLayout;
        uint32_t _xComputeThreads = 0;
        uint32_t _yComputeThreads = 0;
        uint32_t _zComputeThreads = 0;

    };

    ///Represents a stage of shader execution
    class ShaderCode
    {
    public:
        enum class CodeLanguage
        {
            SPIRV,
            DXIL,
            CUSTOM
        };
        ShaderCode(ShaderStageFlags stage, CodeLanguage language, void* data, size_t dataLength);
        ShaderCode(ShaderStageFlags stage, CodeLanguage language, std::filesystem::path path);
        ShaderCode(const ShaderCode&)=delete;
        ShaderCode& operator=(const ShaderCode&)=delete;
        ShaderCode(ShaderCode&& from);
        ShaderCode& operator=(ShaderCode&& from);
        ~ShaderCode();
        ///Raw bytes of shader code
        void* data();
        ///Size in bytes of shader code
        size_t dataSize() const;
        ///Stage of shader pipeline this code represents
        ShaderStageFlags stage() const;
        CodeLanguage language() const;
        ShaderMetaData* metaData();
    private:
        void move(ShaderCode& from);
        ShaderStageFlags _stage;
        CodeLanguage _codeLanguage;
        std::vector<unsigned char> _data;
        ShaderMetaData* _metaData = nullptr;
    };
} // slag

#endif //SLAG_SHADERCODE_H
