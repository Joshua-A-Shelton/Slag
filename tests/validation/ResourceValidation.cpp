// Validation of buffers, memory blocks, textures, and attachment views.

#include "ValidationFixture.h"

using namespace slag;

TEST_CASE("buffers")
{
    ValidationFixture f;
    CHECK_VALIDATION(f, "size is 0", (void)f.device->createBuffer({.size = 0}));
    CHECK_VALIDATION(f, "can be movable",
                     (void)f.device->createBuffer(
                         {.size = 256, .placement = Placement::automatic(MemoryKind::Upload, {.movable = true})}));
    f.common->capabilities.maxBufferSize = 1 << 20;
    CHECK_VALIDATION(f, "exceeds the device's maxBufferSize", (void)f.device->createBuffer({.size = 2 << 20}));
    f.common->capabilities.maxBufferSize = 1ull << 40;

    MemoryBlock* block = f.device->createMemoryBlock({.size = 1 << 20}).value();
    CHECK_VALIDATION(f, "not a multiple of the alignment",
                     (void)f.device->createBuffer({.size = 256, .placement = Placement::placed(block, 256)}));
    CHECK_VALIDATION(f, "ends past the memory block",
                     (void)f.device->createBuffer({.size = 2 << 20, .placement = Placement::placed(block, 0)}));
    ValidationFixture other;
    MemoryBlock* otherBlock = other.device->createMemoryBlock({.size = 1 << 20}).value();
    CHECK_VALIDATION(f, "the memory block belongs to another device",
                     (void)f.device->createBuffer({.size = 256, .placement = Placement::placed(otherBlock, 0)}));
    Buffer* otherBuffer = other.buffer();
    CHECK_VALIDATION(f, "the buffer belongs to another device", f.device->destroy(otherBuffer));
    other.device->destroy(otherBlock);

    Buffer* buffer = f.buffer();
    f.device->destroy(buffer);
    CHECK_VALIDATION(f, "use of a destroyed Buffer", (void)buffer->gpuAddress());
    CHECK_VALIDATION(f, "use of a destroyed Buffer", (void)buffer->mappedData());
    CHECK_VALIDATION(f, "use of a destroyed Buffer", (void)buffer->userData());
    CHECK_VALIDATION(f, "use of a destroyed Buffer", buffer->setUserData(1));
    CHECK_VALIDATION(f, "use of a destroyed Buffer", f.device->destroy(buffer));
    f.device->destroy(block);
    CHECK_VALIDATION(f, "use of a destroyed MemoryBlock",
                     (void)f.device->createBuffer({.size = 256, .placement = Placement::placed(block, 0)}));
}

TEST_CASE("devices that aren't initialized")
{
    ValidationFixture f(false);
    CHECK_VALIDATION(f, "is not initialized", (void)f.device->createBuffer({.size = 256}));
    CHECK_VALIDATION(f, "is not initialized", (void)f.device->placementInfo(BufferDesc{.size = 256}));
    CHECK_VALIDATION(f, "is not initialized", (void)f.device->createMemoryBlock({.size = 1 << 20}));
    CHECK_VALIDATION(f, "is not initialized",
                     (void)f.device->createTexture2D({.format = PixelFormat::R8G8B8A8Unorm, .width = 4, .height = 4}));
    CHECK_VALIDATIONS(f, 2, "is not initialized", (void)f.device->createCommandBuffer({})); // and no queues
    CHECK_VALIDATION(f, "is not initialized", (void)f.device->createSemaphore({}));
    CHECK_VALIDATION(f, "is not initialized", (void)f.device->createQueryPool({.count = 1}));
    CHECK_VALIDATION(f, "is not initialized", (void)f.device->pipelineCacheData({}));
    CHECK_VALIDATION(f, "is not initialized", (void)f.device->beginDefragmentation({}));
    CHECK_VALIDATION(f, "is not initialized", f.device->shutdown());
}

TEST_CASE("memory blocks")
{
    ValidationFixture f;
    CHECK_VALIDATION(f, "size is 0", (void)f.device->createMemoryBlock({.size = 0}));
    f.common->capabilities.gpuUploadMemory = false;
    CHECK_VALIDATION(f, "GpuUpload memory is not supported",
                     (void)f.device->createMemoryBlock({.size = 1 << 20, .kind = MemoryKind::GpuUpload}));
    MemoryBlock* block = f.device->createMemoryBlock({.size = 1 << 20}).value();
    Buffer* placed = f.device->createBuffer({.size = 256, .placement = Placement::placed(block, 0)}).value();
    CHECK_VALIDATION(f, "still holds 1 placed resources", f.device->destroy(block));
    f.device->destroy(placed);
    ValidationFixture other;
    MemoryBlock* otherBlock = other.device->createMemoryBlock({.size = 1 << 20}).value();
    CHECK_VALIDATION(f, "the memory block belongs to another device", f.device->destroy(otherBlock));

    MemoryBlock* doomed = f.device->createMemoryBlock({.size = 1 << 20}).value();
    f.device->destroy(doomed);
    CHECK_VALIDATION(f, "use of a destroyed MemoryBlock", (void)doomed->size());
    CHECK_VALIDATION(f, "use of a destroyed MemoryBlock", (void)doomed->kind());
    CHECK_VALIDATION(f, "use of a destroyed MemoryBlock", (void)doomed->userData());
    CHECK_VALIDATION(f, "use of a destroyed MemoryBlock", doomed->setUserData(1));
    CHECK_VALIDATION(f, "use of a destroyed MemoryBlock", f.device->destroy(doomed));
}

TEST_CASE("texture descriptions")
{
    ValidationFixture f;
    auto texture2D = [&](Texture2DDesc desc) {
        Texture* texture = f.device->createTexture2D(desc).value();
        f.device->destroy(texture);
    };
    auto unsupported = [&](PixelFormat format, FormatSupport removed) {
        f.common->formatSupport[static_cast<size_t>(format)] &= ~removed;
    };
    CHECK_VALIDATION(f, "is not a texture format",
                     texture2D({.format = PixelFormat::Undefined, .width = 4, .height = 4}));
    CHECK_VALIDATION(f, "a size is 0", texture2D({.format = PixelFormat::R8G8B8A8Unorm, .width = 0, .height = 4}));
    CHECK_VALIDATION(
        f, "exceeds the limit",
        texture2D({.format = PixelFormat::R8G8B8A8Unorm, .width = limits::maxTexture2DSize * 2, .height = 4}));
    CHECK_VALIDATION(f, "layer count 0",
                     (void)f.device->createTexture2DArray(
                         {.format = PixelFormat::R8G8B8A8Unorm, .width = 4, .height = 4, .layerCount = 0}));
    CHECK_VALIDATION(f, "mip count 4 is outside",
                     texture2D({.format = PixelFormat::R8G8B8A8Unorm, .width = 4, .height = 4, .mipCount = 4}));
    CHECK_VALIDATION(f, "multiples of 4", texture2D({.format = PixelFormat::BC1Unorm, .width = 6, .height = 4}));
    CHECK_VALIDATION(f, "mutually exclusive",
                     texture2D({.format = PixelFormat::D32Float,
                                .width = 4,
                                .height = 4,
                                .usage = TextureUsage::RenderTarget | TextureUsage::DepthStencil}));
    unsupported(PixelFormat::R8G8B8A8Snorm, FormatSupport::RenderTarget | FormatSupport::DepthStencil |
                                                FormatSupport::ShaderWrite | FormatSupport::Multisample4);
    CHECK_VALIDATION(
        f, "can't be a render target",
        texture2D(
            {.format = PixelFormat::R8G8B8A8Snorm, .width = 4, .height = 4, .usage = TextureUsage::RenderTarget}));
    CHECK_VALIDATION(
        f, "can't be a depth/stencil target",
        texture2D(
            {.format = PixelFormat::R8G8B8A8Snorm, .width = 4, .height = 4, .usage = TextureUsage::DepthStencil}));
    CHECK_VALIDATION(
        f, "can't be written by shaders",
        texture2D({.format = PixelFormat::R8G8B8A8Snorm, .width = 4, .height = 4, .usage = TextureUsage::ShaderWrite}));
    CHECK_VALIDATION(
        f, "depth formats need a 2D or cube texture type",
        (void)f.device->createTexture3D({.format = PixelFormat::D32Float, .width = 4, .height = 4, .depth = 4}));
    ClearValue clear;
    CHECK_VALIDATION(f, "a clear value needs RenderTarget or DepthStencil",
                     texture2D({.format = PixelFormat::R8G8B8A8Unorm, .width = 4, .height = 4, .clearValue = &clear}));

    CHECK_VALIDATION(f, "sample count 3 is not 1, 2, 4, or 8",
                     texture2D({.format = PixelFormat::R8G8B8A8Unorm,
                                .width = 4,
                                .height = 4,
                                .sampleCount = 3,
                                .usage = TextureUsage::RenderTarget}));
    CHECK_VALIDATION(f, "MSAA needs RenderTarget or DepthStencil",
                     texture2D({.format = PixelFormat::R8G8B8A8Unorm, .width = 4, .height = 4, .sampleCount = 4}));
    CHECK_VALIDATION(f, "MSAA textures can't be written by shaders",
                     texture2D({.format = PixelFormat::R8G8B8A8Unorm,
                                .width = 4,
                                .height = 4,
                                .sampleCount = 4,
                                .usage = TextureUsage::RenderTarget | TextureUsage::ShaderWrite}));
    CHECK_VALIDATION(f, "MSAA textures have one mip",
                     texture2D({.format = PixelFormat::R8G8B8A8Unorm,
                                .width = 4,
                                .height = 4,
                                .mipCount = 2,
                                .sampleCount = 4,
                                .usage = TextureUsage::RenderTarget}));
    unsupported(PixelFormat::R8G8B8A8Uint, FormatSupport::Multisample8);
    CHECK_VALIDATION(f, "doesn't support 8x MSAA",
                     texture2D({.format = PixelFormat::R8G8B8A8Uint,
                                .width = 4,
                                .height = 4,
                                .sampleCount = 8,
                                .usage = TextureUsage::RenderTarget}));

    PixelFormat many[17];
    for (PixelFormat& format : many)
    {
        format = PixelFormat::R8G8B8A8UnormSrgb;
    }
    CHECK_VALIDATION(
        f, "more than 16 castable formats",
        texture2D({.format = PixelFormat::R8G8B8A8Unorm, .width = 4, .height = 4, .castableFormats = many}));
    const PixelFormat wrong[] = {PixelFormat::BC1Unorm};
    CHECK_VALIDATION(
        f, "can't be cast to",
        texture2D({.format = PixelFormat::R8G8B8A8Unorm, .width = 4, .height = 4, .castableFormats = wrong}));
    CHECK_VALIDATION(f, "textures need GpuOnly memory",
                     texture2D({.format = PixelFormat::R8G8B8A8Unorm,
                                .width = 4,
                                .height = 4,
                                .placement = Placement::automatic(MemoryKind::Upload)}));
    MemoryBlock* upload = f.device->createMemoryBlock({.size = 1 << 20, .kind = MemoryKind::Upload}).value();
    CHECK_VALIDATION(f, "textures need a GpuOnly memory block",
                     texture2D({.format = PixelFormat::R8G8B8A8Unorm,
                                .width = 4,
                                .height = 4,
                                .placement = Placement::placed(upload, 0)}));
    f.device->destroy(upload);
}

TEST_CASE("texture placement and lifetime")
{
    ValidationFixture f;
    MemoryBlock* block = f.device->createMemoryBlock({.size = 1 << 20}).value();
    auto placed = [&](MemoryBlock* memory, uint64_t offset, uint32_t size) {
        Texture* texture = f.device
                               ->createTexture2D({.format = PixelFormat::R8G8B8A8Unorm,
                                                  .width = size,
                                                  .height = size,
                                                  .placement = Placement::placed(memory, offset)})
                               .value();
        f.device->destroy(texture);
    };
    CHECK_VALIDATION(f, "not a multiple of the alignment", placed(block, 256, 4));
    CHECK_VALIDATION(f, "ends past the memory block", placed(block, 0, 1024));
    ValidationFixture other;
    MemoryBlock* otherBlock = other.device->createMemoryBlock({.size = 1 << 20}).value();
    CHECK_VALIDATION(f, "the memory block belongs to another device", placed(otherBlock, 0, 4));
    other.device->destroy(otherBlock);
    MemoryBlock* doomed = f.device->createMemoryBlock({.size = 1 << 20}).value();
    f.device->destroy(doomed);
    CHECK_VALIDATION(f, "use of a destroyed MemoryBlock", placed(doomed, 0, 4));
    f.device->destroy(block);

    Texture* target = f.texture(TextureUsage::RenderTarget);
    AttachmentView* view = f.device->createAttachmentView({.texture = target}).value();
    CHECK_VALIDATION(f, "still has 1 attachment views", f.device->destroy(target));
    f.device->destroy(view);
    f.device->destroy(target);
    CHECK_VALIDATION(f, "use of a destroyed Texture", (void)target->userData());
    CHECK_VALIDATION(f, "use of a destroyed Texture", target->setUserData(1));
    CHECK_VALIDATION(f, "use of a destroyed Texture", f.device->destroy(target));
    CHECK_VALIDATION(f, "use of a destroyed Texture", (void)f.device->copyFootprint(target, 0));
    Texture* otherTexture = other.texture();
    CHECK_VALIDATION(f, "the texture belongs to another device", f.device->destroy(otherTexture));

    Texture* texture = f.texture();
    CHECK_VALIDATION(f, "texture is null", (void)f.device->copyFootprint(nullptr, 0));
    CHECK_VALIDATION(f, "mip 1 is outside the texture", (void)f.device->copyFootprint(texture, 1));
    Texture* depth = f.texture(TextureUsage::DepthStencil, 4, PixelFormat::D32FloatS8Uint);
    CHECK_VALIDATION(f, "copied one plane at a time", (void)f.device->copyFootprint(depth, 0));
    CHECK_VALIDATION(
        f, "is not a texture format",
        (void)f.device->placementInfo(Texture2DDesc{.format = PixelFormat::Undefined, .width = 4, .height = 4}));
    f.device->destroy(depth);
    f.device->destroy(texture);
}

TEST_CASE("attachment views")
{
    ValidationFixture f;
    CHECK_VALIDATION(f, "texture is null", (void)f.device->createAttachmentView({}));
    Texture* sampled = f.texture();
    CHECK_VALIDATION(f, "neither RenderTarget nor DepthStencil usage",
                     (void)f.device->createAttachmentView({.texture = sampled}));
    Texture* target = f.texture(TextureUsage::RenderTarget);
    CHECK_VALIDATION(f, "is neither the texture's format nor castable",
                     (void)f.device->createAttachmentView({.texture = target, .format = PixelFormat::R32Float}));
    CHECK_VALIDATION(f, "mip 1 is outside the texture",
                     (void)f.device->createAttachmentView({.texture = target, .mip = 1}));
    CHECK_VALIDATION(f, "layers 1..2 are outside the texture",
                     (void)f.device->createAttachmentView({.texture = target, .baseLayer = 1}));
    ValidationFixture other;
    Texture* otherTarget = other.texture(TextureUsage::RenderTarget);
    CHECK_VALIDATION(f, "the texture belongs to another device",
                     (void)f.device->createAttachmentView({.texture = otherTarget}));
    AttachmentView* otherView = other.device->createAttachmentView({.texture = otherTarget}).value();
    CHECK_VALIDATION(f, "the attachment view belongs to another device", f.device->destroy(otherView));
    other.device->destroy(otherView);
    other.device->destroy(otherTarget);

    AttachmentView* view = f.device->createAttachmentView({.texture = target}).value();
    f.device->destroy(view);
    CHECK_VALIDATION(f, "use of a destroyed AttachmentView", (void)view->userData());
    CHECK_VALIDATION(f, "use of a destroyed AttachmentView", view->setUserData(1));
    CHECK_VALIDATION(f, "use of a destroyed AttachmentView", f.device->destroy(view));
    f.device->destroy(target);
    CHECK_VALIDATION(f, "use of a destroyed Texture", (void)f.device->createAttachmentView({.texture = target}));
    f.device->destroy(sampled);
    ValidationFixture uninitialized(false);
    CHECK_VALIDATIONS(uninitialized, 2, "is not initialized", (void)uninitialized.device->createAttachmentView({}));
}
