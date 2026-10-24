#include "common/Logging.h"
#include "common/Objects.h"

#include <slag/Descriptors.h>
#include <slag/GraphicsDevice.h>
#include <slag/Limits.h>

namespace slag {

namespace {

GraphicsDeviceCommon* common(GraphicsDevice* device)
{
    return static_cast<GraphicsDeviceCommon*>(device);
}

#ifdef SLAG_VALIDATION
const char* functionName(BufferDescriptorKind kind)
{
    switch (kind)
    {
    case BufferDescriptorKind::Constant:
        return "GraphicsDevice::writeConstantBuffer";
    case BufferDescriptorKind::Raw:
        return "GraphicsDevice::writeRawBuffer";
    case BufferDescriptorKind::Structured:
        return "GraphicsDevice::writeStructuredBuffer";
    case BufferDescriptorKind::Typed:
        return "GraphicsDevice::writeTypedBuffer";
    case BufferDescriptorKind::RWRaw:
        return "GraphicsDevice::writeRWRawBuffer";
    case BufferDescriptorKind::RWStructured:
        return "GraphicsDevice::writeRWStructuredBuffer";
    case BufferDescriptorKind::RWTyped:
        return "GraphicsDevice::writeRWTypedBuffer";
    }
    return "GraphicsDevice::write";
}

void validateBuffer(GraphicsDeviceCommon* self, uint32_t index, const BufferDescriptorInfo& info)
{
    InstanceCommon* instance = self->instance;
    const char* function = functionName(info.kind);
    SLAG_VALIDATE(instance, index < self->resourceHeapSize, function, "slot %u is outside the resource heap (%u slots)",
                  index, self->resourceHeapSize);
    if (info.kind == BufferDescriptorKind::Constant)
    {
        SLAG_VALIDATE(instance, info.address % alignment::constantBuffer == 0, function,
                      "the address is not a multiple of %u", alignment::constantBuffer);
        SLAG_VALIDATE(
            instance,
            info.size > 0 && info.size % alignment::constantBuffer == 0 && info.size <= limits::maxConstantBufferSize,
            function, "size %llu is not a multiple of %u up to %u", static_cast<unsigned long long>(info.size),
            alignment::constantBuffer, limits::maxConstantBufferSize);
        return;
    }
    SLAG_VALIDATE(instance, info.buffer, function, "buffer is null");
    if (!info.buffer)
    {
        return;
    }
    SLAG_VALIDATE(instance, info.offset + info.size <= info.buffer->size, function, "the view ends past the buffer");
    switch (info.kind)
    {
    case BufferDescriptorKind::Raw:
    case BufferDescriptorKind::RWRaw:
        SLAG_VALIDATE(instance, info.offset % alignment::rawBuffer == 0, function, "offset is not a multiple of %u",
                      alignment::rawBuffer);
        SLAG_VALIDATE(instance, info.size > 0 && info.size % 4 == 0 && info.size <= limits::maxStorageBufferViewSize,
                      function, "size must be a non-zero multiple of 4 up to limits::maxStorageBufferViewSize");
        break;
    case BufferDescriptorKind::Structured:
    case BufferDescriptorKind::RWStructured:
        SLAG_VALIDATE(instance, info.stride > 0 && info.stride % 4 == 0, function,
                      "stride %u must be a non-zero multiple of 4", info.stride);
        SLAG_VALIDATE(instance, info.stride == 0 || info.offset % info.stride == 0, function,
                      "offset must be a multiple of the stride");
        SLAG_VALIDATE(instance,
                      info.count > 0 && info.count <= limits::maxStructuredBufferElements &&
                          info.size <= limits::maxStorageBufferViewSize,
                      function, "count %u is 0 or exceeds the structured buffer view limits", info.count);
        break;
    case BufferDescriptorKind::Typed:
    case BufferDescriptorKind::RWTyped:
    {
        FormatSupport needed =
            info.kind == BufferDescriptorKind::Typed ? FormatSupport::TypedBuffer : FormatSupport::RWTypedBuffer;
        SLAG_VALIDATE(instance, (self->formatSupport[static_cast<size_t>(info.format)] & needed) != FormatSupport::None,
                      function, "%s can't be used in this kind of typed buffer", formatInfo(info.format).name);
        SLAG_VALIDATE(instance, info.offset % alignment::rawBuffer == 0, function, "offset is not a multiple of %u",
                      alignment::rawBuffer);
        SLAG_VALIDATE(instance, info.count > 0 && info.count <= limits::maxTypedBufferElements, function,
                      "count %u is 0 or exceeds limits::maxTypedBufferElements", info.count);
        break;
    }
    case BufferDescriptorKind::Constant:
        break;
    }
}

bool viewTypeMatches(TextureType type, TextureViewType view, uint32_t sampleCount)
{
    switch (type)
    {
    case TextureType::Texture1D:
        return view == TextureViewType::Texture1D;
    case TextureType::Texture1DArray:
        return view == TextureViewType::Texture1DArray;
    case TextureType::Texture2D:
        return view == (sampleCount > 1 ? TextureViewType::Texture2DMS : TextureViewType::Texture2D);
    case TextureType::Texture2DArray:
        return view == (sampleCount > 1 ? TextureViewType::Texture2DMSArray : TextureViewType::Texture2DArray);
    case TextureType::Texture3D:
        return view == TextureViewType::Texture3D;
    case TextureType::TextureCube:
        return view == TextureViewType::TextureCube || view == TextureViewType::Texture2DArray;
    case TextureType::TextureCubeArray:
        return view == TextureViewType::TextureCubeArray || view == TextureViewType::Texture2DArray;
    }
    return false;
}

bool castableTo(const TextureCommon& texture, PixelFormat format)
{
    if (format == texture.format)
    {
        return true;
    }
    for (uint32_t i = 0; i < texture.castableCount; ++i)
    {
        if (texture.castableFormats[i] == format)
        {
            return true;
        }
    }
    return false;
}

void validateTexture(GraphicsDeviceCommon* self, uint32_t index, const TextureDescriptorInfo& info)
{
    InstanceCommon* instance = self->instance;
    const char* function = info.writable ? "GraphicsDevice::writeRWTexture" : "GraphicsDevice::writeTexture";
    SLAG_VALIDATE(instance, index < self->resourceHeapSize, function, "slot %u is outside the resource heap (%u slots)",
                  index, self->resourceHeapSize);
    const TextureCommon* texture = info.texture;
    SLAG_VALIDATE(instance, texture, function, "texture is null");
    if (!texture)
    {
        return;
    }
    SLAG_VALIDATE(instance, viewTypeMatches(texture->type, info.viewType, texture->sampleCount), function,
                  "the view type doesn't match the texture's type");
    SLAG_VALIDATE(instance, !info.writable || texture->sampleCount == 1, function, "MSAA textures can't be written");
    SLAG_VALIDATE(instance, !info.writable || (texture->usage & TextureUsage::ShaderWrite) != TextureUsage::None,
                  function, "the texture was created without ShaderWrite");
    SLAG_VALIDATE(instance, castableTo(*texture, info.format), function,
                  "%s is neither the texture's format nor castable", formatInfo(info.format).name);
    SLAG_VALIDATE(instance,
                  info.baseMip < texture->mipCount && info.mipCount >= 1 &&
                      info.mipCount <= texture->mipCount - info.baseMip,
                  function, "the mip range is outside the texture (%u mips)", texture->mipCount);
    uint32_t layers = texture->type == TextureType::Texture3D ? 1 : texture->layerCount;
    SLAG_VALIDATE(instance,
                  info.baseLayer < layers && info.layerCount >= 1 && info.layerCount <= layers - info.baseLayer,
                  function, "the layer range is outside the texture (%u layers)", layers);
    if (info.viewType == TextureViewType::TextureCube || info.viewType == TextureViewType::TextureCubeArray)
    {
        SLAG_VALIDATE(instance, info.baseLayer % 6 == 0 && info.layerCount % 6 == 0, function,
                      "cube views cover whole cubes");
        SLAG_VALIDATE(instance, info.viewType != TextureViewType::TextureCube || info.layerCount == 6, function,
                      "a TextureCube view covers one cube");
    }
    bool stencil = hasTraits(texture->format, FormatTraits::Stencil);
    bool depth = hasTraits(texture->format, FormatTraits::Depth);
    SLAG_VALIDATE(instance,
                  (info.plane == TexturePlane::Color && !stencil) || (info.plane == TexturePlane::Depth && depth) ||
                      (info.plane == TexturePlane::Stencil && stencil),
                  function, "the plane doesn't fit the format (depth/stencil textures name one plane)");
}

void validateSampler(GraphicsDeviceCommon* self, uint32_t index, const SamplerDescriptor& descriptor)
{
    InstanceCommon* instance = self->instance;
    SLAG_VALIDATE(instance, index < self->samplerHeapSize, "GraphicsDevice::writeSampler",
                  "slot %u is outside the sampler heap (%u slots)", index, self->samplerHeapSize);
    SLAG_VALIDATE(instance, descriptor.maxAnisotropy >= 1 && descriptor.maxAnisotropy <= limits::maxAnisotropy,
                  "GraphicsDevice::writeSampler", "maxAnisotropy %u is outside 1..%u", descriptor.maxAnisotropy,
                  limits::maxAnisotropy);
    SLAG_VALIDATE(instance,
                  descriptor.maxAnisotropy == 1 ||
                      (descriptor.minFilter == Filter::Linear && descriptor.magFilter == Filter::Linear &&
                       descriptor.mipFilter == Filter::Linear),
                  "GraphicsDevice::writeSampler", "anisotropic filtering needs every filter Linear");
    SLAG_VALIDATE(instance,
                  descriptor.mipLodBias >= -limits::maxSamplerLodBias &&
                      descriptor.mipLodBias <= limits::maxSamplerLodBias,
                  "GraphicsDevice::writeSampler", "mipLodBias is outside +-%g", limits::maxSamplerLodBias);
    SLAG_VALIDATE(instance, descriptor.minLod <= descriptor.maxLod, "GraphicsDevice::writeSampler",
                  "minLod is above maxLod");
    SLAG_VALIDATE(instance, descriptor.reductionMode == ReductionMode::Standard || self->capabilities.samplerReduction,
                  "GraphicsDevice::writeSampler", "min/max reduction is not supported on \"%s\"", self->info.name);
    SLAG_VALIDATE(instance, !descriptor.compareEnable || descriptor.reductionMode == ReductionMode::Standard,
                  "GraphicsDevice::writeSampler", "comparison samplers use Standard reduction");
}
#endif

void writeBuffer(GraphicsDeviceCommon* self, uint32_t index, const BufferDescriptorInfo& info)
{
#ifdef SLAG_VALIDATION
    validateBuffer(self, index, info);
#endif
    SLAG_DISPATCH(self, writeBufferDescriptor)(self, index, info);
}

BufferDescriptorInfo bufferInfo(BufferDescriptorKind kind, const ConstantBufferDescriptor& descriptor)
{
    BufferDescriptorInfo info;
    info.kind = kind;
    info.address = descriptor.address;
    info.size = descriptor.size;
    return info;
}

BufferDescriptorInfo bufferInfo(BufferDescriptorKind kind, const RawBufferDescriptor& descriptor)
{
    BufferDescriptorInfo info;
    info.kind = kind;
    info.buffer = static_cast<BufferCommon*>(descriptor.buffer);
    info.address = info.buffer ? info.buffer->gpuAddress + descriptor.offset : 0;
    info.offset = descriptor.offset;
    info.size = descriptor.size;
    return info;
}

BufferDescriptorInfo bufferInfo(BufferDescriptorKind kind, const StructuredBufferDescriptor& descriptor)
{
    BufferDescriptorInfo info;
    info.kind = kind;
    info.buffer = static_cast<BufferCommon*>(descriptor.buffer);
    info.address = info.buffer ? info.buffer->gpuAddress + descriptor.offset : 0;
    info.offset = descriptor.offset;
    info.size = uint64_t(descriptor.count) * descriptor.stride;
    info.count = descriptor.count;
    info.stride = descriptor.stride;
    return info;
}

BufferDescriptorInfo bufferInfo(BufferDescriptorKind kind, const TypedBufferDescriptor& descriptor)
{
    BufferDescriptorInfo info;
    info.kind = kind;
    info.buffer = static_cast<BufferCommon*>(descriptor.buffer);
    info.address = info.buffer ? info.buffer->gpuAddress + descriptor.offset : 0;
    info.offset = descriptor.offset;
    info.count = descriptor.count;
    info.format = descriptor.format;
    info.size = uint64_t(descriptor.count) * formatInfo(descriptor.format).blockBytes;
    return info;
}

TextureDescriptorInfo textureInfo(const TextureDescriptor& descriptor)
{
    TextureDescriptorInfo info;
    info.texture = static_cast<TextureCommon*>(descriptor.texture);
    info.viewType = descriptor.viewType;
    info.baseMip = descriptor.baseMip;
    info.baseLayer = descriptor.baseLayer;
    info.plane = descriptor.plane;
    if (info.texture)
    {
        info.format = descriptor.format == PixelFormat::Undefined ? info.texture->format : descriptor.format;
        uint32_t layers = info.texture->type == TextureType::Texture3D ? 1 : info.texture->layerCount;
        info.mipCount = descriptor.mipCount == SubresourceRange::remainingMips
                            ? info.texture->mipCount - descriptor.baseMip
                            : descriptor.mipCount;
        info.layerCount = descriptor.layerCount == SubresourceRange::remainingLayers ? layers - descriptor.baseLayer
                                                                                     : descriptor.layerCount;
    }
    return info;
}

TextureDescriptorInfo textureInfo(const RWTextureDescriptor& descriptor)
{
    TextureDescriptorInfo info;
    info.texture = static_cast<TextureCommon*>(descriptor.texture);
    info.writable = true;
    info.viewType = descriptor.viewType;
    info.baseMip = descriptor.mip;
    info.mipCount = 1;
    info.baseLayer = descriptor.baseLayer;
    if (info.texture)
    {
        info.format = descriptor.format == PixelFormat::Undefined ? info.texture->format : descriptor.format;
        uint32_t layers = info.texture->type == TextureType::Texture3D ? 1 : info.texture->layerCount;
        info.layerCount = descriptor.layerCount == SubresourceRange::remainingLayers ? layers - descriptor.baseLayer
                                                                                     : descriptor.layerCount;
    }
    return info;
}

void writeTextureInfo(GraphicsDeviceCommon* self, uint32_t index, const TextureDescriptorInfo& info)
{
#ifdef SLAG_VALIDATION
    validateTexture(self, index, info);
#endif
    SLAG_DISPATCH(self, writeTextureDescriptor)(self, index, info);
}

} // namespace

#define SLAG_BUFFER_WRITES(Function, Kind, Descriptor)                                                                 \
    void GraphicsDevice::Function(uint32_t index, const Descriptor& descriptor)                                        \
    {                                                                                                                  \
        writeBuffer(common(this), index, bufferInfo(BufferDescriptorKind::Kind, descriptor));                          \
    }                                                                                                                  \
    void GraphicsDevice::Function(std::span<const Descriptor##Write> writes)                                           \
    {                                                                                                                  \
        for (const Descriptor##Write& write : writes)                                                                  \
        {                                                                                                              \
            writeBuffer(common(this), write.index, bufferInfo(BufferDescriptorKind::Kind, write.descriptor));          \
        }                                                                                                              \
    }

SLAG_BUFFER_WRITES(writeConstantBuffer, Constant, ConstantBufferDescriptor)
SLAG_BUFFER_WRITES(writeRawBuffer, Raw, RawBufferDescriptor)
SLAG_BUFFER_WRITES(writeStructuredBuffer, Structured, StructuredBufferDescriptor)
SLAG_BUFFER_WRITES(writeTypedBuffer, Typed, TypedBufferDescriptor)
SLAG_BUFFER_WRITES(writeRWRawBuffer, RWRaw, RawBufferDescriptor)
SLAG_BUFFER_WRITES(writeRWStructuredBuffer, RWStructured, StructuredBufferDescriptor)
SLAG_BUFFER_WRITES(writeRWTypedBuffer, RWTyped, TypedBufferDescriptor)

#undef SLAG_BUFFER_WRITES

void GraphicsDevice::writeTexture(uint32_t index, const TextureDescriptor& descriptor)
{
    writeTextureInfo(common(this), index, textureInfo(descriptor));
}

void GraphicsDevice::writeTexture(std::span<const TextureDescriptorWrite> writes)
{
    for (const TextureDescriptorWrite& write : writes)
    {
        writeTextureInfo(common(this), write.index, textureInfo(write.descriptor));
    }
}

void GraphicsDevice::writeRWTexture(uint32_t index, const RWTextureDescriptor& descriptor)
{
    writeTextureInfo(common(this), index, textureInfo(descriptor));
}

void GraphicsDevice::writeRWTexture(std::span<const RWTextureDescriptorWrite> writes)
{
    for (const RWTextureDescriptorWrite& write : writes)
    {
        writeTextureInfo(common(this), write.index, textureInfo(write.descriptor));
    }
}

void GraphicsDevice::writeSampler(uint32_t index, const SamplerDescriptor& descriptor)
{
    GraphicsDeviceCommon* self = common(this);
#ifdef SLAG_VALIDATION
    validateSampler(self, index, descriptor);
#endif
    SLAG_DISPATCH(self, writeSamplerDescriptor)(self, index, descriptor);
}

void GraphicsDevice::writeSampler(std::span<const SamplerDescriptorWrite> writes)
{
    for (const SamplerDescriptorWrite& write : writes)
    {
        writeSampler(write.index, write.descriptor);
    }
}

// Slot tracking and D3D12 poison descriptors arrive with milestone M7 (ADR 0031).
void GraphicsDevice::clearResourceDescriptor([[maybe_unused]] uint32_t index)
{
    [[maybe_unused]] GraphicsDeviceCommon* self = common(this);
    SLAG_VALIDATE(self->instance, index < self->resourceHeapSize, "GraphicsDevice::clearResourceDescriptor",
                  "slot %u is outside the resource heap (%u slots)", index, self->resourceHeapSize);
}

void GraphicsDevice::clearSamplerDescriptor([[maybe_unused]] uint32_t index)
{
    [[maybe_unused]] GraphicsDeviceCommon* self = common(this);
    SLAG_VALIDATE(self->instance, index < self->samplerHeapSize, "GraphicsDevice::clearSamplerDescriptor",
                  "slot %u is outside the sampler heap (%u slots)", index, self->samplerHeapSize);
}

} // namespace slag
