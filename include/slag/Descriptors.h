#pragma once

// Descriptor structs for resource and sampler heap writes (ADRs 0030, 0031, 0033).
// Each write function has a batch form taking {index, descriptor} pairs.

#include <slag/Barriers.h>
#include <slag/PixelFormat.h>
#include <slag/Types.h>

#include <cstdint>

namespace slag {

class Buffer;
class Texture;

// ConstantBuffer<T>. address and size are multiples of
// alignment::constantBuffer (256); size is at most limits::maxConstantBufferSize.
struct ConstantBufferDescriptor
{
    uint64_t address = 0;
    uint32_t size = 0;
};

// ByteAddressBuffer / RWByteAddressBuffer. offset is a multiple of
// alignment::rawBuffer (256), size a multiple of 4, at most
// limits::maxStorageBufferViewSize.
struct RawBufferDescriptor
{
    Buffer* buffer = nullptr;
    uint64_t offset = 0;
    uint64_t size = 0;
};

// StructuredBuffer<T> / RWStructuredBuffer<T>. offset must be a multiple of
// stride; count * stride bytes must fit the buffer.
struct StructuredBufferDescriptor
{
    Buffer* buffer = nullptr;
    uint64_t offset = 0;
    uint32_t count = 0;
    uint32_t stride = 0;
};

// Buffer<T> / RWBuffer<T>. offset is a multiple of alignment::rawBuffer
// (256); count elements of `format`, at most limits::maxTypedBufferElements.
struct TypedBufferDescriptor
{
    Buffer* buffer = nullptr;
    uint64_t offset = 0;
    uint32_t count = 0;
    PixelFormat format = PixelFormat::Undefined;
};

enum class TextureViewType : uint8_t
{
    Texture1D,
    Texture1DArray,
    Texture2D,
    Texture2DArray,
    Texture2DMS,
    Texture2DMSArray,
    Texture3D,
    TextureCube,
    TextureCubeArray,
};

// Texture1D, Texture2D, TextureCube, ... The view type matches the texture's
// type; cubes may also be viewed as 2D arrays. Sampled textures are in the
// ShaderRead or DepthStencilRead layout. Depth/stencil textures name the
// plane to read; single-plane depth formats may use Color.
struct TextureDescriptor
{
    Texture* texture = nullptr;
    TextureViewType viewType = TextureViewType::Texture2D;
    PixelFormat format = PixelFormat::Undefined; // Undefined: the texture's format; else a castable format
    uint32_t baseMip = 0;
    uint32_t mipCount = SubresourceRange::remainingMips;
    uint32_t baseLayer = 0; // cube arrays: whole cubes (multiples of 6)
    uint32_t layerCount = SubresourceRange::remainingLayers;
    TexturePlane plane = TexturePlane::Color;
};

// RWTexture1D, RWTexture2D, ... One mip, in the ShaderWrite layout. 3D
// textures are written whole (Vulkan has no depth-slice range for 3D storage
// views).
struct RWTextureDescriptor
{
    Texture* texture = nullptr;
    TextureViewType viewType = TextureViewType::Texture2D;
    PixelFormat format = PixelFormat::Undefined;
    uint32_t mip = 0;
    uint32_t baseLayer = 0;
    uint32_t layerCount = SubresourceRange::remainingLayers;
};

enum class Filter : uint8_t
{
    Point,
    Linear,
};

enum class AddressMode : uint8_t
{
    Wrap,
    Mirror,
    Clamp,
    Border,
    MirrorOnce,
};

enum class BorderColor : uint8_t
{
    TransparentBlack,
    OpaqueBlack,
    OpaqueWhite,
};

// Minimum and Maximum need GraphicsDeviceCapabilities::samplerReduction.
enum class ReductionMode : uint8_t
{
    Standard,
    Minimum,
    Maximum,
};

// ADR 0033. maxAnisotropy above 1 needs every filter Linear.
struct SamplerDescriptor
{
    Filter minFilter = Filter::Linear;
    Filter magFilter = Filter::Linear;
    Filter mipFilter = Filter::Linear;
    AddressMode addressU = AddressMode::Wrap;
    AddressMode addressV = AddressMode::Wrap;
    AddressMode addressW = AddressMode::Wrap;
    float mipLodBias = 0.0f;    // within +-limits::maxSamplerLodBias
    uint32_t maxAnisotropy = 1; // 1 (off) to limits::maxAnisotropy
    bool compareEnable = false;
    CompareOp compareOp = CompareOp::Never;
    float minLod = 0.0f;
    float maxLod = 1000.0f;
    BorderColor borderColor = BorderColor::TransparentBlack;
    ReductionMode reductionMode = ReductionMode::Standard;
};

// Batch-write entries.
struct ConstantBufferDescriptorWrite
{
    uint32_t index = 0;
    ConstantBufferDescriptor descriptor;
};

struct RawBufferDescriptorWrite
{
    uint32_t index = 0;
    RawBufferDescriptor descriptor;
};

struct StructuredBufferDescriptorWrite
{
    uint32_t index = 0;
    StructuredBufferDescriptor descriptor;
};

struct TypedBufferDescriptorWrite
{
    uint32_t index = 0;
    TypedBufferDescriptor descriptor;
};

struct TextureDescriptorWrite
{
    uint32_t index = 0;
    TextureDescriptor descriptor;
};

struct RWTextureDescriptorWrite
{
    uint32_t index = 0;
    RWTextureDescriptor descriptor;
};

struct SamplerDescriptorWrite
{
    uint32_t index = 0;
    SamplerDescriptor descriptor;
};

} // namespace slag
