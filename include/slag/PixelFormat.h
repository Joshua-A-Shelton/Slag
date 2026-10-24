#pragma once

// PixelFormat and FormatSupport (ADR 0022, docs/format-support.md).

#include <slag/Flags.h>

#include <cstdint>

namespace slag {

// Bit layouts follow DXGI (ADR 0022).
enum class PixelFormat : uint8_t
{
    Undefined,

    R8Unorm,
    R8Snorm,
    R8Uint,
    R8Sint,
    R8G8Unorm,
    R8G8Snorm,
    R8G8Uint,
    R8G8Sint,
    R8G8B8A8Unorm,
    R8G8B8A8UnormSrgb,
    R8G8B8A8Snorm,
    R8G8B8A8Uint,
    R8G8B8A8Sint,
    B8G8R8A8Unorm,
    B8G8R8A8UnormSrgb,

    R16Unorm,
    R16Snorm,
    R16Uint,
    R16Sint,
    R16Float,
    R16G16Unorm,
    R16G16Snorm,
    R16G16Uint,
    R16G16Sint,
    R16G16Float,
    R16G16B16A16Unorm,
    R16G16B16A16Snorm,
    R16G16B16A16Uint,
    R16G16B16A16Sint,
    R16G16B16A16Float,

    R32Uint,
    R32Sint,
    R32Float,
    R32G32Uint,
    R32G32Sint,
    R32G32Float,
    R32G32B32Uint,  // vertex attributes only (ADR 0038)
    R32G32B32Sint,  // vertex attributes only
    R32G32B32Float, // vertex attributes only
    R32G32B32A32Uint,
    R32G32B32A32Sint,
    R32G32B32A32Float,

    R10G10B10A2Unorm,
    R10G10B10A2Uint,
    R11G11B10Float,
    R9G9B9E5SharedExp, // sampling only

    D16Unorm,
    D32Float,
    D32FloatS8Uint,

    BC1Unorm,
    BC1UnormSrgb,
    BC3Unorm,
    BC3UnormSrgb,
    BC4Unorm,
    BC4Snorm,
    BC5Unorm,
    BC5Snorm,
    BC6HUfloat16,
    BC6HSfloat16,
    BC7Unorm,
    BC7UnormSrgb,
};

// What a device supports for one format (the columns of
// docs/format-support.md). Copies work for every sampled format.
enum class FormatSupport : uint32_t
{
    None = 0,
    Sampled = 1u << 0,      // sampled or loaded in shaders
    LinearFilter = 1u << 1, // linear filtering
    RenderTarget = 1u << 2,
    Blend = 1u << 3,           // blending as a render target
    Multisample2 = 1u << 4,    // 2x MSAA render or depth target
    Multisample4 = 1u << 5,    // 4x
    Multisample8 = 1u << 6,    // 8x
    ShaderWrite = 1u << 7,     // RWTexture* stores
    ShaderWriteLoad = 1u << 8, // shader reads of a writable texture
    Atomics = 1u << 9,         // atomics on RWTexture* / RWBuffer
    DepthStencil = 1u << 10,   // depth/stencil target
    VertexBuffer = 1u << 11,   // vertex attribute
    TypedBuffer = 1u << 12,    // Buffer<T>
    RWTypedBuffer = 1u << 13,  // RWBuffer<T>
};
SLAG_FLAGS(FormatSupport)

} // namespace slag
