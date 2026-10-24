#include "common/Logging.h"
#include "common/Objects.h"
#include "common/SubAllocator.h"

#include <slag/GraphicsDevice.h>
#include <slag/Limits.h>
#include <slag/Texture.h>

#include <bit>

namespace slag {

namespace {

template <class Desc>
TextureCreateInfo commonFields(TextureType type, const Desc& desc)
{
    TextureCreateInfo info;
    info.type = type;
    info.format = desc.format;
    info.mipCount = desc.mipCount;
    info.usage = desc.usage;
    info.castableFormats = desc.castableFormats;
    info.clearValue = desc.clearValue;
    info.placement = desc.placement;
    info.debugName = desc.debugName;
    info.userData = desc.userData;
    return info;
}

TextureCreateInfo toInfo(const Texture1DDesc& desc)
{
    TextureCreateInfo info = commonFields(TextureType::Texture1D, desc);
    info.width = desc.width;
    return info;
}

TextureCreateInfo toInfo(const Texture1DArrayDesc& desc)
{
    TextureCreateInfo info = commonFields(TextureType::Texture1DArray, desc);
    info.width = desc.width;
    info.layerCount = desc.layerCount;
    return info;
}

TextureCreateInfo toInfo(const Texture2DDesc& desc)
{
    TextureCreateInfo info = commonFields(TextureType::Texture2D, desc);
    info.width = desc.width;
    info.height = desc.height;
    info.sampleCount = desc.sampleCount;
    return info;
}

TextureCreateInfo toInfo(const Texture2DArrayDesc& desc)
{
    TextureCreateInfo info = commonFields(TextureType::Texture2DArray, desc);
    info.width = desc.width;
    info.height = desc.height;
    info.layerCount = desc.layerCount;
    info.sampleCount = desc.sampleCount;
    return info;
}

TextureCreateInfo toInfo(const Texture3DDesc& desc)
{
    TextureCreateInfo info = commonFields(TextureType::Texture3D, desc);
    info.width = desc.width;
    info.height = desc.height;
    info.depth = desc.depth;
    return info;
}

TextureCreateInfo toInfo(const TextureCubeDesc& desc)
{
    TextureCreateInfo info = commonFields(TextureType::TextureCube, desc);
    info.width = desc.size;
    info.height = desc.size;
    info.layerCount = 6;
    return info;
}

TextureCreateInfo toInfo(const TextureCubeArrayDesc& desc)
{
    TextureCreateInfo info = commonFields(TextureType::TextureCubeArray, desc);
    info.width = desc.size;
    info.height = desc.size;
    info.layerCount = 6 * desc.cubeCount;
    return info;
}

[[maybe_unused]] uint32_t maxSize(TextureType type)
{
    switch (type)
    {
    case TextureType::Texture1D:
    case TextureType::Texture1DArray:
        return limits::maxTexture1DSize;
    case TextureType::Texture2D:
    case TextureType::Texture2DArray:
        return limits::maxTexture2DSize;
    case TextureType::Texture3D:
        return limits::maxTexture3DSize;
    case TextureType::TextureCube:
    case TextureType::TextureCubeArray:
        return limits::maxTextureCubeSize;
    }
    return 0;
}

[[maybe_unused]] bool hasUsage(TextureUsage usage, TextureUsage flag)
{
    return (usage & flag) != TextureUsage::None;
}

[[maybe_unused]] bool supports(const GraphicsDeviceCommon* device, PixelFormat format, FormatSupport flag)
{
    return (device->formatSupport[static_cast<size_t>(format)] & flag) != FormatSupport::None;
}

[[maybe_unused]] FormatSupport multisampleSupport(uint32_t sampleCount)
{
    switch (sampleCount)
    {
    case 2:
        return FormatSupport::Multisample2;
    case 4:
        return FormatSupport::Multisample4;
    case 8:
        return FormatSupport::Multisample8;
    default:
        return FormatSupport::None;
    }
}

// Validation shared by create and placementInfo (ADRs 0021, 0022, 0028).
void validateTexture(GraphicsDeviceCommon* self, const TextureCreateInfo& info, const char* function)
{
#ifdef SLAG_VALIDATION
    InstanceCommon* instance = self->instance;
    const FormatInfo& format = formatInfo(info.format);
    SLAG_VALIDATE(instance, self->initialized, function, "device \"%s\" is not initialized", self->info.name);
    SLAG_VALIDATE(instance,
                  info.format != PixelFormat::Undefined && supports(self, info.format, FormatSupport::Sampled),
                  function, "%s is not a texture format", format.name);
    uint32_t limit = maxSize(info.type);
    SLAG_VALIDATE(instance, info.width >= 1 && info.height >= 1 && info.depth >= 1, function, "a size is 0");
    SLAG_VALIDATE(instance, info.width <= limit && info.height <= limit && info.depth <= limit, function,
                  "size %ux%ux%u exceeds the limit (%u)", info.width, info.height, info.depth, limit);
    SLAG_VALIDATE(instance, info.layerCount >= 1 && info.layerCount <= limits::maxTextureArrayLayers, function,
                  "layer count %u is outside 1..%u", info.layerCount, limits::maxTextureArrayLayers);
    uint32_t largest = info.width > info.height ? info.width : info.height;
    largest = largest > info.depth ? largest : info.depth;
    uint32_t fullChain = static_cast<uint32_t>(std::bit_width(largest));
    SLAG_VALIDATE(instance, info.mipCount >= 1 && info.mipCount <= fullChain && info.mipCount <= limits::maxMipLevels,
                  function, "mip count %u is outside 1..%u", info.mipCount,
                  fullChain < limits::maxMipLevels ? fullChain : limits::maxMipLevels);
    SLAG_VALIDATE(instance,
                  !hasTraits(info.format, FormatTraits::Compressed) || (info.width % 4 == 0 && info.height % 4 == 0),
                  function, "block-compressed textures need a width and height that are multiples of 4");

    bool renderTarget = hasUsage(info.usage, TextureUsage::RenderTarget);
    bool depthStencil = hasUsage(info.usage, TextureUsage::DepthStencil);
    SLAG_VALIDATE(instance, !(renderTarget && depthStencil), function,
                  "RenderTarget and DepthStencil are mutually exclusive");
    SLAG_VALIDATE(instance, !renderTarget || supports(self, info.format, FormatSupport::RenderTarget), function,
                  "%s can't be a render target", format.name);
    SLAG_VALIDATE(instance, !depthStencil || supports(self, info.format, FormatSupport::DepthStencil), function,
                  "%s can't be a depth/stencil target", format.name);
    SLAG_VALIDATE(instance,
                  !hasUsage(info.usage, TextureUsage::ShaderWrite) ||
                      supports(self, info.format, FormatSupport::ShaderWrite),
                  function, "%s can't be written by shaders", format.name);
    SLAG_VALIDATE(instance,
                  !hasTraits(info.format, FormatTraits::Depth) ||
                      (info.type != TextureType::Texture1D && info.type != TextureType::Texture1DArray &&
                       info.type != TextureType::Texture3D),
                  function, "depth formats need a 2D or cube texture type");
    SLAG_VALIDATE(instance, !info.clearValue || renderTarget || depthStencil, function,
                  "a clear value needs RenderTarget or DepthStencil");

    if (info.sampleCount != 1)
    {
        FormatSupport msaa = multisampleSupport(info.sampleCount);
        SLAG_VALIDATE(instance, msaa != FormatSupport::None, function, "sample count %u is not 1, 2, 4, or 8",
                      info.sampleCount);
        SLAG_VALIDATE(instance, renderTarget || depthStencil, function, "MSAA needs RenderTarget or DepthStencil");
        SLAG_VALIDATE(instance, !hasUsage(info.usage, TextureUsage::ShaderWrite), function,
                      "MSAA textures can't be written by shaders");
        SLAG_VALIDATE(instance, info.mipCount == 1, function, "MSAA textures have one mip");
        SLAG_VALIDATE(instance, msaa == FormatSupport::None || supports(self, info.format, msaa), function,
                      "%s doesn't support %ux MSAA", format.name, info.sampleCount);
    }

    SLAG_VALIDATE(instance, info.castableFormats.size() <= 16, function, "more than 16 castable formats");
    for (PixelFormat castable : info.castableFormats)
    {
        SLAG_VALIDATE(instance, formatsCastable(info.format, castable), function, "%s can't be cast to %s", format.name,
                      formatInfo(castable).name);
    }

    // Textures live in GpuOnly memory (ADR 0023).
    if (info.placement.memoryBlock)
    {
        auto* block = static_cast<const MemoryBlockCommon*>(info.placement.memoryBlock);
        SLAG_VALIDATE(instance, block->kind == MemoryKind::GpuOnly, function, "textures need a GpuOnly memory block");
    }
    else
    {
        SLAG_VALIDATE(instance, info.placement.kind == MemoryKind::GpuOnly, function, "textures need GpuOnly memory");
    }
#else
    (void)self;
    (void)info;
    (void)function;
#endif
}

Result<Texture*> createTexture(GraphicsDeviceCommon* self, const TextureCreateInfo& info, const char* function)
{
    validateTexture(self, info, function);
#ifdef SLAG_VALIDATION
    SLAG_CHECK_LIVE(self->instance, MemoryBlockCommon, info.placement.memoryBlock, "MemoryBlock", function);
    if (info.placement.memoryBlock)
    {
        auto* block = static_cast<const MemoryBlockCommon*>(info.placement.memoryBlock);
        PlacementInfo placement = SLAG_DISPATCH(self, texturePlacementInfo)(self, info);
        SLAG_VALIDATE(self->instance, block->device == self, function, "the memory block belongs to another device");
        SLAG_VALIDATE(self->instance, placement.alignment && info.placement.offset % placement.alignment == 0, function,
                      "offset %llu is not a multiple of the alignment (%llu)",
                      static_cast<unsigned long long>(info.placement.offset),
                      static_cast<unsigned long long>(placement.alignment));
        SLAG_VALIDATE(self->instance,
                      info.placement.offset <= block->size && placement.size <= block->size - info.placement.offset,
                      function, "the texture (%llu bytes at offset %llu) ends past the memory block",
                      static_cast<unsigned long long>(placement.size),
                      static_cast<unsigned long long>(info.placement.offset));
    }
#endif
    if (self->lost)
    {
        return Status::DeviceLost;
    }

    // Automatic placement: sub-allocated, or dedicated when large (ADR 0061).
    PoolAllocation allocation;
    TextureCreateInfo placed = info;
    if (!info.placement.memoryBlock)
    {
        PlacementInfo placement = SLAG_DISPATCH(self, texturePlacementInfo)(self, info);
        if (self->subAllocator->fits(MemoryKind::GpuOnly, placement))
        {
            MemoryBlockCommon* block = nullptr;
            uint64_t offset = 0;
            Status status = self->subAllocator->allocate(MemoryKind::GpuOnly, placement, info.placement.movable,
                                                         allocation, block, offset);
            if (status != Status::Ok)
            {
                return status;
            }
            placed.placement = Placement::placed(block, offset);
        }
    }
    Result<TextureCommon*> texture = SLAG_DISPATCH(self, createTexture)(self, placed);
    if (!texture)
    {
        self->subAllocator->free(allocation);
        return texture.status();
    }
    TextureCommon* object = texture.value();
    object->userData = info.userData;
    setDebugName(self->instance, object, info.debugName);
    object->type = info.type;
    object->format = info.format;
    object->width = info.width;
    object->height = info.height;
    object->depth = info.depth;
    object->layerCount = info.layerCount;
    object->mipCount = info.mipCount;
    object->sampleCount = info.sampleCount;
    object->usage = info.usage;
    if (info.clearValue)
    {
        object->clearValue = *info.clearValue;
        object->hasClearValue = true;
    }
    for (PixelFormat castable : info.castableFormats)
    {
        if (object->castableCount < 16)
        {
            object->castableFormats[object->castableCount++] = castable;
        }
    }
    if (info.placement.memoryBlock)
    {
        object->memoryBlock = static_cast<MemoryBlockCommon*>(info.placement.memoryBlock);
        ++object->memoryBlock->liveResources;
    }
    if (allocation.block)
    {
        object->allocation = allocation;
        self->subAllocator->setOwner(allocation, object, true);
    }
    return static_cast<Texture*>(object);
}

PlacementInfo placementInfo(const GraphicsDeviceCommon* self, const TextureCreateInfo& info)
{
    validateTexture(const_cast<GraphicsDeviceCommon*>(self), info, "GraphicsDevice::placementInfo");
    return SLAG_DISPATCH(self, texturePlacementInfo)(self, info);
}

GraphicsDeviceCommon* common(GraphicsDevice* device)
{
    return static_cast<GraphicsDeviceCommon*>(device);
}

const GraphicsDeviceCommon* common(const GraphicsDevice* device)
{
    return static_cast<const GraphicsDeviceCommon*>(device);
}

} // namespace

Result<Texture*> GraphicsDevice::createTexture1D(const Texture1DDesc& desc)
{
    return createTexture(common(this), toInfo(desc), "GraphicsDevice::createTexture1D");
}

Result<Texture*> GraphicsDevice::createTexture1DArray(const Texture1DArrayDesc& desc)
{
    return createTexture(common(this), toInfo(desc), "GraphicsDevice::createTexture1DArray");
}

Result<Texture*> GraphicsDevice::createTexture2D(const Texture2DDesc& desc)
{
    return createTexture(common(this), toInfo(desc), "GraphicsDevice::createTexture2D");
}

Result<Texture*> GraphicsDevice::createTexture2DArray(const Texture2DArrayDesc& desc)
{
    return createTexture(common(this), toInfo(desc), "GraphicsDevice::createTexture2DArray");
}

Result<Texture*> GraphicsDevice::createTexture3D(const Texture3DDesc& desc)
{
    return createTexture(common(this), toInfo(desc), "GraphicsDevice::createTexture3D");
}

Result<Texture*> GraphicsDevice::createTextureCube(const TextureCubeDesc& desc)
{
    return createTexture(common(this), toInfo(desc), "GraphicsDevice::createTextureCube");
}

Result<Texture*> GraphicsDevice::createTextureCubeArray(const TextureCubeArrayDesc& desc)
{
    return createTexture(common(this), toInfo(desc), "GraphicsDevice::createTextureCubeArray");
}

void GraphicsDevice::destroy(Texture* texture)
{
    SLAG_CHECK_LIVE(static_cast<const GraphicsDeviceCommon*>(this)->instance, TextureCommon, texture, "Texture",
                    "GraphicsDevice::destroy");
    if (!texture)
    {
        return;
    }
    GraphicsDeviceCommon* self = common(this);
    auto* object = static_cast<TextureCommon*>(texture);
    SLAG_VALIDATE(self->instance, object->device == self, "GraphicsDevice::destroy",
                  "the texture belongs to another device");
    SLAG_VALIDATE(self->instance, object->liveViews == 0, "GraphicsDevice::destroy",
                  "the texture still has %u attachment views", object->liveViews.load());
    SLAG_VALIDATE(self->instance, !object->movePass, "GraphicsDevice::destroy",
                  "the texture is being moved by the active defragmentation pass; its move is dropped");
    if (object->movePass)
    {
        dropMove(object->movePass, object->moveIndex);
    }
    MemoryBlockCommon* block = object->memoryBlock;
    PoolAllocation allocation = object->allocation;
    SLAG_DISPATCH(self, destroyTexture)(self, object);
    if (block)
    {
        --block->liveResources;
    }
    self->subAllocator->free(allocation);
}

PlacementInfo GraphicsDevice::placementInfo(const Texture1DDesc& desc) const
{
    return slag::placementInfo(common(this), toInfo(desc));
}

PlacementInfo GraphicsDevice::placementInfo(const Texture1DArrayDesc& desc) const
{
    return slag::placementInfo(common(this), toInfo(desc));
}

PlacementInfo GraphicsDevice::placementInfo(const Texture2DDesc& desc) const
{
    return slag::placementInfo(common(this), toInfo(desc));
}

PlacementInfo GraphicsDevice::placementInfo(const Texture2DArrayDesc& desc) const
{
    return slag::placementInfo(common(this), toInfo(desc));
}

PlacementInfo GraphicsDevice::placementInfo(const Texture3DDesc& desc) const
{
    return slag::placementInfo(common(this), toInfo(desc));
}

PlacementInfo GraphicsDevice::placementInfo(const TextureCubeDesc& desc) const
{
    return slag::placementInfo(common(this), toInfo(desc));
}

PlacementInfo GraphicsDevice::placementInfo(const TextureCubeArrayDesc& desc) const
{
    return slag::placementInfo(common(this), toInfo(desc));
}

CopyFootprint GraphicsDevice::copyFootprint(const Texture* texture, uint32_t mip, TexturePlane plane) const
{
    SLAG_CHECK_LIVE(static_cast<const GraphicsDeviceCommon*>(this)->instance, TextureCommon, texture, "Texture",
                    "GraphicsDevice::copyFootprint");
    [[maybe_unused]] const GraphicsDeviceCommon* self = common(this);
    auto* object = static_cast<const TextureCommon*>(texture);
    SLAG_VALIDATE(self->instance, object, "GraphicsDevice::copyFootprint", "texture is null");
    if (!object)
    {
        return {};
    }
    SLAG_VALIDATE(self->instance, mip < object->mipCount, "GraphicsDevice::copyFootprint",
                  "mip %u is outside the texture (%u mips)", mip, object->mipCount);
    SLAG_VALIDATE(self->instance, !hasTraits(object->format, FormatTraits::Stencil) || plane != TexturePlane::Color,
                  "GraphicsDevice::copyFootprint", "depth/stencil textures are copied one plane at a time");
    CopyFootprint footprint;
    footprint.width = object->width >> mip ? object->width >> mip : 1;
    footprint.height = object->height >> mip ? object->height >> mip : 1;
    footprint.depth = object->type == TextureType::Texture3D ? (object->depth >> mip ? object->depth >> mip : 1) : 1;
    uint32_t block = blockDimension(object->format);
    uint32_t blocksWide = (footprint.width + block - 1) / block;
    footprint.rowCount = (footprint.height + block - 1) / block;
    uint32_t rowBytes = blocksWide * copyBlockBytes(object->format, plane);
    footprint.rowPitch = (rowBytes + alignment::textureCopyRowPitch - 1) / alignment::textureCopyRowPitch *
                         alignment::textureCopyRowPitch;
    footprint.size = uint64_t(footprint.rowPitch) * footprint.rowCount * footprint.depth;
    return footprint;
}

uint64_t Texture::userData() const
{
    SLAG_CHECK_LIVE_THIS(TextureCommon, "Texture", "Texture::userData");
    return static_cast<const TextureCommon*>(this)->userData;
}

void Texture::setUserData(uint64_t value)
{
    SLAG_CHECK_LIVE_THIS(TextureCommon, "Texture", "Texture::setUserData");
    static_cast<TextureCommon*>(this)->userData = value;
}

} // namespace slag
