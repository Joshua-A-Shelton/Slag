#pragma once

// Pipeline, pipeline Descs, state structs, ShaderStage, VertexInputLayout (ADRs 0036–0043).

#include <slag/Flags.h>
#include <slag/PixelFormat.h>
#include <slag/Types.h>

#include <cstddef>
#include <cstdint>
#include <span>

namespace slag {

// Values from 0x80 up are reserved for external backends (ADR 0036).
enum class ShaderFormat : uint8_t
{
    Dxil,
    Spirv,
};

struct ShaderBinary
{
    ShaderFormat format = ShaderFormat::Spirv;
    const void* data = nullptr;
    size_t size = 0;
};

// The first binary the backend accepts is used (ADR 0036). Only needs to live
// until the create call returns.
struct ShaderStage
{
    std::span<const ShaderBinary> binaries;
    const char* entryPoint = "main";
};

// --- Vertex input (ADR 0038) -------------------------------------------------

enum class VertexStepRate : uint8_t
{
    PerVertex,
    PerInstance, // one step per instance
};

// One vertex shader input. `location` is the input's position among the
// shader's non-system inputs (declaration order), or its [[vk::location]].
struct VertexAttribute
{
    uint32_t location = 0;
    PixelFormat format = PixelFormat::Undefined;
    uint32_t slot = 0;   // vertex buffer slot
    uint32_t offset = 0; // bytes into the vertex; always explicit
};

// Empty for vertex pulling. Strides are set when vertex buffers are bound.
struct VertexInputLayout
{
    std::span<const VertexAttribute> attributes;
    std::span<const VertexStepRate> slots; // one per slot used, from slot 0
};

// --- Fixed-function state (ADRs 0039-0042) -----------------------------------

enum class PrimitiveTopology : uint8_t
{
    PointList,
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip,
};

enum class FillMode : uint8_t
{
    Solid,
    Wireframe,
};

enum class CullMode : uint8_t
{
    None,
    Front,
    Back,
};

struct RasterizerState
{
    FillMode fillMode = FillMode::Solid;
    CullMode cullMode = CullMode::None;
    bool frontCounterClockwise = false;
    float depthBias = 0.0f;
    float depthBiasClamp = 0.0f;
    float depthBiasSlopeScale = 0.0f;
    bool depthClip = true;                  // off: depth is clamped instead
    bool conservativeRasterization = false; // needs GraphicsDeviceCapabilities::conservativeRasterization
};

enum class StencilOp : uint8_t
{
    Keep,
    Zero,
    Replace,
    IncrementSaturate,
    DecrementSaturate,
    Invert,
    IncrementWrap,
    DecrementWrap,
};

struct StencilFaceState
{
    StencilOp failOp = StencilOp::Keep;
    StencilOp depthFailOp = StencilOp::Keep;
    StencilOp passOp = StencilOp::Keep;
    CompareOp compareOp = CompareOp::Always;
};

struct DepthStencilState
{
    bool depthTest = false;
    bool depthWrite = false;
    CompareOp depthCompareOp = CompareOp::Less;
    bool stencilTest = false;
    StencilFaceState front;
    StencilFaceState back;
    uint8_t stencilReadMask = 0xFF; // shared by both faces
    uint8_t stencilWriteMask = 0xFF;
    bool depthBoundsTest = false; // needs GraphicsDeviceCapabilities::depthBounds; bounds set while recording
};

enum class BlendFactor : uint8_t
{
    Zero,
    One,
    SrcColor,
    InvSrcColor,
    SrcAlpha,
    InvSrcAlpha,
    SrcAlphaSaturate,
    DestColor,
    InvDestColor,
    DestAlpha,
    InvDestAlpha,
    BlendConstant,
    InvBlendConstant,
    Src1Color, // dual-source: render target 0 only
    InvSrc1Color,
    Src1Alpha,
    InvSrc1Alpha,
};

enum class BlendOp : uint8_t
{
    Add,
    Subtract,
    ReverseSubtract,
    Min,
    Max,
};

enum class ColorWriteMask : uint8_t
{
    None = 0,
    Red = 1u << 0,
    Green = 1u << 1,
    Blue = 1u << 2,
    Alpha = 1u << 3,
    All = Red | Green | Blue | Alpha,
};
SLAG_FLAGS(ColorWriteMask)

struct ColorTargetBlend
{
    bool blendEnable = false;
    BlendFactor srcColorFactor = BlendFactor::One;
    BlendFactor destColorFactor = BlendFactor::Zero;
    BlendOp colorOp = BlendOp::Add;
    BlendFactor srcAlphaFactor = BlendFactor::One;
    BlendFactor destAlphaFactor = BlendFactor::Zero;
    BlendOp alphaOp = BlendOp::Add;
    ColorWriteMask writeMask = ColorWriteMask::All;
};

// Always independent per target (ADR 0041).
struct BlendState
{
    bool alphaToCoverage = false;
    ColorTargetBlend targets[8];
};

// A vertex shader and an optional pixel shader (no binaries: depth-only).
// Target formats and sample count must match the attachment views used when
// rendering (ADR 0042).
struct GraphicsPipelineDesc
{
    ShaderStage vertex;
    ShaderStage pixel;
    VertexInputLayout vertexInput;
    PrimitiveTopology topology = PrimitiveTopology::TriangleList;
    RasterizerState rasterizer;
    DepthStencilState depthStencil;
    BlendState blend;
    std::span<const PixelFormat> colorFormats; // up to 8; Undefined leaves a gap
    PixelFormat depthStencilFormat = PixelFormat::Undefined;
    uint32_t sampleCount = 1;
    uint32_t sampleMask = 0xFFFFFFFF;
    const char* debugName = nullptr;
    uint64_t userData = 0;
};

struct ComputePipelineDesc
{
    ShaderStage compute;
    const char* debugName = nullptr;
    uint64_t userData = 0;
};

class Pipeline
{
public:
    uint64_t userData() const;
    void setUserData(uint64_t value);

    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;

protected:
    Pipeline() = default;
    ~Pipeline() = default;
};

} // namespace slag
