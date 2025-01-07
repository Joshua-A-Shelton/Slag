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
    ///Details about the rasterization of pixels
    struct RasterizationState
    {
        ///How to fill the geometry with pixels
        enum DrawMode
        {
            ///Draw the entire triangle
            FACE,
            ///Draw just the edges of the triangle
            EDGE,
            ///Draw just the vertices of the triangle
            VERTEX
        };
        ///Which triangles to cull (not draw)
        enum CullOptions
        {
            NONE,
            FRONT_FACING,
            BACK_FACING
        };
        ///Which triangles are considered front facing for culling
        enum FrontFacing
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
        DrawMode drawMode = FACE;
        ///Which faces of a mesh to not draw
        CullOptions culling = BACK_FACING;
        ///What winding order determines a face as front facing
        FrontFacing frontFacing = CLOCKWISE;
        ///Enable depth biasing for drawn fragments
        bool depthBiasEnable = false;
        ///Extra nudge to bias fragments in the depth buffer if depthBiasEnable is true
        int depthBiasConstantFactor = 0;
        ///Maximum (or minimum) depth bias of a fragment
        float depthBiasClamp = 0.0f;
        ///Scalar applied to a fragments slope in depth bias calculations
        float depthBiasSlopeFactor = 0.0f;
        ///How thick to draw rasterized lines (DrawModes other than FILL)
        float lineWidth = 1.0f;
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
        Color::ComponentFlags colorWriteMask = Color::RED_COMPONENT | Color::GREEN_COMPONENT | Color::BLUE_COMPONENT | Color::ALPHA_COMPONENT;
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
        Operations::ComparisonFunction depthCompareOperation = Operations::COMPARISION_LESS;
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

} // slag

#endif //SLAG_SHADERPROPERTIES_H
