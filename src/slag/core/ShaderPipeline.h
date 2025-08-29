#ifndef SLAG_SHADERPIPELINE_H
#define SLAG_SHADERPIPELINE_H

#include <cstdint>
#include <filesystem>
#include <vector>

#include "Color.h"
#include "Color.h"
#include "Operations.h"
#include "Pixels.h"
#include "VertexDescription.h"

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
    ///Represents a stage of shader execution
    class ShaderCode
    {
    public:
        enum class CodeLanguage
        {
            SPIRV,
            CUSTOM
        };
        ShaderCode(ShaderStageFlags stage, CodeLanguage language, void* data, size_t dataLength);
        ShaderCode(ShaderStageFlags stage, CodeLanguage language, std::filesystem::path path);
        ShaderCode(const ShaderCode&)=delete;
        ShaderCode& operator=(const ShaderCode&)=delete;
        ShaderCode(ShaderCode&& from);
        ShaderCode& operator=(ShaderCode&& from);
        ///Raw bytes of shader code
        void* data();
        ///Size in bytes of shader code
        size_t dataSize();
        ///Stage of shader pipeline this code represents
        ShaderStageFlags stage();
        CodeLanguage language();
    private:
        void move(ShaderCode& from);
        ShaderStageFlags _stage;
        CodeLanguage _codeLanguage;
        std::vector<unsigned char> _data;
    };


    ///Details about the rasterization of pixels
    struct RasterizationState
    {
        ///How to fill the geometry with pixels
        enum class DrawMode: uint8_t
        {
            ///Draw the entire triangle
            FACE,
            ///Draw just the edges of the triangle
            EDGE,
            ///Draw just the vertices of the triangle
            VERTEX
        };
        ///Which triangles to cull (not draw)
        enum class CullOptions: uint8_t
        {
            NONE,
            FRONT_FACING,
            BACK_FACING
        };
        ///Which triangles are considered front facing for culling
        enum class FrontFacing: uint8_t
        {
            ///Triangles that specify vertices in a clockwise manner
            CLOCKWISE,
            ///Triangles that specify vertices in a counter clockwise manner
            COUNTER_CLOCKWISE
        };
        ///Whether or not a fragment's depth is clamped to values in the depth test
        bool depthClampEnable = false;
        ///Whether or not geometry is discarded before the rasterization stage
        bool rasterizerDicardEnable = false;
        ///How to fill drawn geometry
        DrawMode drawMode = DrawMode::FACE;
        ///How thick to draw rasterized lines (DrawModes EDGE only)
        float lineThickness = 1.0f;
        ///Which faces of a mesh to not draw
        CullOptions culling = CullOptions::BACK_FACING;
        ///What winding order determines a face as front facing
        FrontFacing frontFacing = FrontFacing::CLOCKWISE;

        ///Enable depth biasing for drawn fragments
        bool depthBiasEnable = false;
        ///Extra nudge to bias fragments in the depth buffer if depthBiasEnable is true
        int depthBiasConstantFactor = 0;
        ///Maximum (or minimum) depth bias of a fragment
        float depthBiasClamp = 0.0f;
        ///Scalar applied to a fragments slope in depth bias calculations
        float depthBiasSlopeFactor = 0.0f;

    };
    ///Details about multisampling
    struct MultiSampleState
    {
        ///Number or rasterization samples (1/2/4/8/16)
        uint8_t rasterizationSamples = 1;
        ///Enable sample shading (require multiple samples to generate a fragment)
        bool sampleShadingEnable = false;
        ///Minimum number of samples (1/2/4/8/16) needed to generate a fragment is sampleShadingEnable is true
        uint8_t minSampleShading = 1;
        ///Controls if an alpha component of a fragment's first color is replaced in multisampling
        bool alphaToOneEnable = false;
    };
    ///Details about blending partially transparent pixels per Attachment
    struct BlendAttachmentState
    {
        ///Enable blending
        bool blendingEnabled = true;
        ///Blend factor for RGB components of source texel
        Operations::BlendFactor srcColorBlendFactor = Operations::BlendFactor::BLEND_FACTOR_SRC_ALPHA;
        ///Blend factor from RGB components of incoming texel
        Operations::BlendFactor dstColorBlendFactor = Operations::BlendFactor::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        ///Operation for combining RGB components of mixing texels
        Operations::BlendOperation colorBlendOperation = Operations::BlendOperation::BLEND_OP_ADD;
        ///Blend factor for Alpha component of source texel
        Operations::BlendFactor srcAlphaBlendFactor = Operations::BlendFactor::BLEND_FACTOR_ONE;
        ///Blend factor for Alpha component of incoming texel
        Operations::BlendFactor dstAlphaBlendFactor = Operations::BlendFactor::BLEND_FACTOR_ZERO;
        ///Operation for combining Alpha components of mixing texels
        Operations::BlendOperation alphaBlendOperation = Operations::BlendOperation::BLEND_OP_ADD;
        ///Mask to only write certain components of the texels
        Color::ComponentFlags colorWriteMask = Color::ComponentFlags::RED_COMPONENT | Color::ComponentFlags::GREEN_COMPONENT | Color::ComponentFlags::BLUE_COMPONENT | Color::ComponentFlags::ALPHA_COMPONENT;
    };
    ///Details about blending partially transparent pixels
    struct BlendState
    {
        ///Whether to apply logical operation to determine if blending should occur
        bool logicOperationEnable = false;
        ///What operation to perform to determine if blending should occur
        Operations::LogicalOperation logicalOperation = Operations::LogicalOperation::LOGIC_OP_COPY;
        ///Blend states for color attachments, up to 8, that correspond to FrameBufferDescription color attachments
        BlendAttachmentState attachmentBlendStates[8]{};
    };

    //TODO: set to sensible defaults
    ///Parameter on stencil usage
    struct StencilOpState
    {
        ///Action performed on samples that fail the stencil test
        Operations::StencilOperation failOp = Operations::StencilOperation::STENCIL_OP_KEEP;
        ///Action performed on samples that pass the stencil test
        Operations::StencilOperation passOp = Operations::StencilOperation::STENCIL_OP_KEEP;
        ///Action performed on samples that pass the stencil test, but fail the depth test
        Operations::StencilOperation depthFailOp = Operations::StencilOperation::STENCIL_OP_KEEP;
        ///Comparison to use in the stencil test
        Operations::ComparisonFunction compareOp = Operations::ComparisonFunction::COMPARISION_NEVER;
    };
    ///Details on the depth stencil usage
    struct DepthStencilState
    {
        ///Whether to perform a depth test or not
        bool depthTestEnable = true;
        ///Whether to update the depth buffer if a fragment passes the depth test with that fragment's depth
        bool depthWriteEnable = true;
        ///Operation to use to determine if a fragment passes the depth test
        Operations::ComparisonFunction depthCompareOperation = Operations::ComparisonFunction::COMPARISION_LESS;
        ///Whether or not to enable stencil buffer testing
        bool stencilTestEnable = false;
        ///Which bits of the stencil part of the buffer are part of the stencil test
        uint8_t stencilReadMask = 0xFF;
        ///Which bits of the stencil buffer are update by the stencil test
        uint8_t stencilWriteMask = 0xFF;
        ///Parameter for stencil test on front facing polygons
        StencilOpState front{};
        ///Parameter for stencil test on back facing polygons
        StencilOpState back{};

    };
    ///Collection of properties that describe how a shader should behave
    class ShaderProperties
    {
    public:
        ///Details about how to rasterize fragments
        RasterizationState rasterizationState{};
        ///Details about how to handle multi sampling
        MultiSampleState multiSampleState{};
        ///Details about blending transparent pixels
        BlendState blendState{};
        ///Details about depth/stencil tests
        DepthStencilState depthStencilState{};
    };
    ///Description for a shader of what it's render targets/depth buffer will be
    class FrameBufferDescription
    {
    public:
        ///Color target pixel formats, Pixels::Format::UNDEFINED means no target at that slot
        Pixels::Format colorTargets[8]{Pixels::Format::UNDEFINED};
        ///Depth target format, Pixels::Format::UNDEFINED means no depth target
        Pixels::Format depthTarget{Pixels::Format::UNDEFINED};
    };

    enum class ShaderPipelineType
    {
        GRAPHICS,
        COMPUTE
    };

    ///Collection of shaders that get executed in order to perform operations on the graphics card
    class ShaderPipeline
    {
    public:
        virtual ~ShaderPipeline()=default;
        ///What kind of shader pipeline this is
        virtual ShaderPipelineType pipelineType()=0;
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

        ///Number of compute threads in the x dimension (0 for graphics pipelines)
        virtual uint32_t xComputeThreads()=0;
        ///Number of compute threads in the y dimension (0 for graphics pipelines)
        virtual uint32_t yComputeThreads()=0;
        ///Number of compute threads in the z dimension (0 for graphics pipelines)
        virtual uint32_t zComputeThreads()=0;

        ///Shader languages the current backend can accept to create a shader pipeline
        static std::vector<ShaderCode::CodeLanguage> acceptedLanguages();

        static ShaderPipeline* newShaderPipeline(ShaderCode** shaders, size_t shaderCount, ShaderProperties& properties, VertexDescription& vertexDescription, FrameBufferDescription& framebufferDescription);
        static ShaderPipeline* newShaderPipeline(const ShaderCode& computeShader);

    };
} // slag

#endif //SLAG_SHADERPIPELINE_H
