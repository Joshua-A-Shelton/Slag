#ifndef SLAG_SHADERPROPERTIES_H
#define SLAG_SHADERPROPERTIES_H

#include "GraphicsTypes.h"
#include "Pixel.h"
#include "Color.h"
#include "Sampler.h"
#include "Operations.h"
#include <cstdint>
#include <cassert>
#include <vector>

namespace slag
{
    struct RasterizationState
    {
        enum DrawMode
        {
            FACE,
            EDGE,
            VERTEX
        };
        enum CullOptions
        {
            NONE,
            FRONT_FACING,
            BACK_FACING
        };
        enum FrontFacing
        {
            CLOCKWISE,
            COUNTER_CLOCKWISE
        };
        bool depthClampEnable = false;
        bool rasterizerDicardEnable = false;
        DrawMode drawMode = FACE;
        CullOptions culling = BACK_FACING;
        FrontFacing frontFacing = CLOCKWISE;
        bool depthBiasEnable = false;
        float depthBiasConstantFactor = 0.0f;
        float depthBiasClamp = 0.0f;
        float depthBiasSlopeFactor = 0.0f;
        float lineWidth = 1.0f;
    };

    struct MultiSampleState
    {
        uint8_t rasterizationSamples = 1;
        bool sampleShadingEnable = false;
        float minSampleShading = 1.0f;
        bool alphaToOneEnable = false;
    };

    struct BlendAttachmentState
    {
        bool blendingEnabled = true;
        Operations::BlendFactor srcColorBlendFactor = Operations::BlendFactor::BLEND_FACTOR_SRC_ALPHA;
        Operations::BlendFactor dstColorBlendFactor = Operations::BlendFactor::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        Operations::BlendOperation colorBlendOperation = Operations::BlendOperation::BLEND_OP_ADD;
        Operations::BlendFactor srcAlphaBlendFactor = Operations::BlendFactor::BLEND_FACTOR_ONE;
        Operations::BlendFactor dstAlphaBlendFactor = Operations::BlendFactor::BLEND_FACTOR_ZERO;
        Operations::BlendOperation alphaBlendOperation = Operations::BlendOperation::BLEND_OP_ADD;
        Color::ComponentFlags colorWriteMask = Color::RED_COMPONENT | Color::GREEN_COMPONENT | Color::BLUE_COMPONENT | Color::ALPHA_COMPONENT;
    };

    struct BlendState
    {
        bool logicOperationEnable = false;
        Operations::LogicalOperation logicalOperation = Operations::LogicalOperation::LOGIC_OP_COPY;
        ///Blend states for color attachments, up to 8, that correspond to FrameBufferDescription
        BlendAttachmentState attachmentBlendStates[8]{};
        float blendConstants[4]{0.0f};
    };

    //TODO: set to sensible defaults
    struct StencilOpState
    {
        Operations::StencilOperation failOp = Operations::StencilOperation::STENCIL_OP_KEEP;
        Operations::StencilOperation passOp = Operations::StencilOperation::STENCIL_OP_KEEP;
        Operations::StencilOperation depthFailOp = Operations::StencilOperation::STENCIL_OP_KEEP;
        Operations::ComparisonFunction compareOp = Operations::ComparisonFunction::COMPARISION_NEVER;
        uint32_t compareMask = 0;
        uint32_t writeMask = 0;
        uint32_t reference = 0;
    };

    struct DepthStencilState
    {
        bool depthTestEnable = true;
        bool depthWriteEnable = true;
        Operations::ComparisonFunction depthCompareOperation = Operations::COMPARISION_LESS;
        bool depthBoundsTestEnable = false;
        bool stencilTestEnable = false;
        StencilOpState front{};
        StencilOpState back{};
        float minDepthBounds = 0;
        float maxDepthBounds = 1;
    };

    class ShaderProperties
    {
    public:
        RasterizationState rasterizationState{};
        MultiSampleState multiSampleState{};
        BlendState blendState{};
        DepthStencilState depthStencilState{};
    };

} // slag

#endif //SLAG_SHADERPROPERTIES_H
