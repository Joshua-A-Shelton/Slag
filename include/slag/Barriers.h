#pragma once

// BarrierSync, BarrierAccess, TextureLayout, barrier structs (ADR 0049).

#include <slag/Flags.h>

#include <cstdint>

namespace slag {

class Buffer;
class Texture;

enum class BarrierSync : uint32_t
{
    None = 0,
    All = 1u << 0,
    Draw = 1u << 1,
    IndexInput = 1u << 2,
    VertexShading = 1u << 3,
    PixelShading = 1u << 4,
    DepthStencil = 1u << 5,
    RenderTarget = 1u << 6,
    ComputeShading = 1u << 7,
    AllShading = 1u << 8,
    NonPixelShading = 1u << 9,
    Copy = 1u << 10,
    Resolve = 1u << 11,
    ExecuteIndirect = 1u << 12,
};
SLAG_FLAGS(BarrierSync)

enum class BarrierAccess : uint32_t
{
    NoAccess = 0,
    VertexBuffer = 1u << 0,
    IndexBuffer = 1u << 1,
    ConstantBuffer = 1u << 2,
    ShaderRead = 1u << 3,
    ShaderWrite = 1u << 4,
    RenderTarget = 1u << 5,
    DepthStencilRead = 1u << 6,
    DepthStencilWrite = 1u << 7,
    IndirectArgument = 1u << 8,
    CopySource = 1u << 9,
    CopyDest = 1u << 10,
    ResolveSource = 1u << 11,
    ResolveDest = 1u << 12,
};
SLAG_FLAGS(BarrierAccess)

enum class TextureLayout : uint8_t
{
    Undefined,
    Common,
    Present,
    RenderTarget,
    DepthStencilWrite,
    DepthStencilRead,
    ShaderRead,
    ShaderWrite,
    CopySource,
    CopyDest,
    ResolveSource,
    ResolveDest,
};

enum class TexturePlane : uint8_t
{
    Color,
    Depth,
    Stencil,
};

struct SubresourceRange
{
    uint32_t baseMip = 0;
    uint32_t mipCount = UINT32_MAX; // UINT32_MAX: to the last mip
    uint32_t baseLayer = 0;
    uint32_t layerCount = UINT32_MAX;
    TexturePlane plane = TexturePlane::Color;
};

struct GlobalBarrier
{
    BarrierSync syncBefore = BarrierSync::None;
    BarrierSync syncAfter = BarrierSync::None;
    BarrierAccess accessBefore = BarrierAccess::NoAccess;
    BarrierAccess accessAfter = BarrierAccess::NoAccess;
};

// Covers the whole buffer (ADR 0049).
struct BufferBarrier
{
    BarrierSync syncBefore = BarrierSync::None;
    BarrierSync syncAfter = BarrierSync::None;
    BarrierAccess accessBefore = BarrierAccess::NoAccess;
    BarrierAccess accessAfter = BarrierAccess::NoAccess;
    Buffer* buffer = nullptr;
};

struct TextureBarrier
{
    BarrierSync syncBefore = BarrierSync::None;
    BarrierSync syncAfter = BarrierSync::None;
    BarrierAccess accessBefore = BarrierAccess::NoAccess;
    BarrierAccess accessAfter = BarrierAccess::NoAccess;
    TextureLayout layoutBefore = TextureLayout::Undefined; // Undefined discards the contents
    TextureLayout layoutAfter = TextureLayout::Undefined;
    Texture* texture = nullptr;
    SubresourceRange range;
};

} // namespace slag
