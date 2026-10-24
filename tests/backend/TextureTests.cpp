#include "TestDevice.h"

#include <slag/AttachmentView.h>
#include <slag/Barriers.h>
#include <slag/CommandBuffer.h>
#include <slag/GraphicsDevice.h>
#include <slag/Semaphore.h>
#include <slag/Texture.h>

#include <vector>

namespace {

using slag::PixelFormat;
using slag::TextureUsage;

// Every texture type, a few usages, automatic and placed, and a transition of
// each through the layouts copies and sampling use.
void testTextureCreation(slag::Backend backend)
{
    TestInstance test(backend);
    slag::GraphicsDevice* device = test.firstDevice();
    REQUIRE(device);
    REQUIRE(static_cast<bool>(device->initialize({})));

    const PixelFormat srgb[] = {PixelFormat::R8G8B8A8UnormSrgb};
    const PixelFormat sameSize[] = {PixelFormat::R32Uint};
    slag::ClearValue clearColor = {.color = {0.0f, 0.5f, 1.0f, 1.0f}};
    slag::ClearValue clearDepth = {.depth = 1.0f};

    std::vector<slag::Texture*> textures;
    auto keep = [&](slag::Result<slag::Texture*> result) {
        REQUIRE(static_cast<bool>(result));
        textures.push_back(result.value());
    };
    keep(device->createTexture1D({.format = PixelFormat::R8G8B8A8Unorm, .width = 256, .mipCount = 9}));
    keep(device->createTexture1DArray(
        {.format = PixelFormat::R16G16B16A16Float, .width = 64, .layerCount = 4, .usage = TextureUsage::ShaderWrite}));
    keep(device->createTexture2D({.format = PixelFormat::R8G8B8A8Unorm,
                                  .width = 512,
                                  .height = 256,
                                  .mipCount = 10,
                                  .usage = TextureUsage::RenderTarget,
                                  .castableFormats = srgb,
                                  .clearValue = &clearColor,
                                  .debugName = "color"}));
    keep(device->createTexture2D({.format = PixelFormat::D32FloatS8Uint,
                                  .width = 512,
                                  .height = 256,
                                  .usage = TextureUsage::DepthStencil,
                                  .clearValue = &clearDepth}));
    keep(device->createTexture2D({.format = PixelFormat::R8G8B8A8Unorm,
                                  .width = 256,
                                  .height = 256,
                                  .sampleCount = 4,
                                  .usage = TextureUsage::RenderTarget}));
    keep(device->createTexture2D({.format = PixelFormat::BC7Unorm, .width = 128, .height = 64, .mipCount = 3}));
    keep(device->createTexture2DArray({.format = PixelFormat::R32Float,
                                       .width = 128,
                                       .height = 128,
                                       .layerCount = 3,
                                       .usage = TextureUsage::ShaderWrite,
                                       .castableFormats = sameSize}));
    keep(device->createTexture3D({.format = PixelFormat::R8G8B8A8Unorm,
                                  .width = 32,
                                  .height = 32,
                                  .depth = 16,
                                  .mipCount = 6,
                                  .usage = TextureUsage::RenderTarget}));
    keep(device->createTextureCube(
        {.format = PixelFormat::R11G11B10Float, .size = 64, .mipCount = 7, .usage = TextureUsage::RenderTarget}));
    keep(
        device->createTextureCube({.format = PixelFormat::D16Unorm, .size = 128, .usage = TextureUsage::DepthStencil}));
    keep(device->createTextureCubeArray(
        {.format = PixelFormat::R8G8B8A8UnormSrgb, .size = 32, .cubeCount = 2, .mipCount = 2}));

    // Placed: two textures in one block, at offsets from placementInfo.
    slag::Texture2DDesc placedDesc = {.format = PixelFormat::R16Float, .width = 300, .height = 200, .mipCount = 2};
    slag::PlacementInfo placement = device->placementInfo(placedDesc);
    CHECK(placement.size > 0);
    CHECK(placement.alignment >= 4096);
    slag::MemoryBlock* block = device->createMemoryBlock({.size = 8 << 20}).value();
    REQUIRE(block);
    uint64_t second = (placement.size + placement.alignment - 1) / placement.alignment * placement.alignment;
    placedDesc.placement = slag::Placement::placed(block, 0);
    keep(device->createTexture2D(placedDesc));
    placedDesc.placement = slag::Placement::placed(block, second);
    keep(device->createTexture2D(placedDesc));
    // A tiny texture may use 4 KB alignment; MSAA needs 4 MB.
    CHECK(device->placementInfo(slag::Texture2DDesc{.format = PixelFormat::R8Unorm, .width = 16, .height = 16})
              .alignment <= 65536);
    CHECK(device
              ->placementInfo(slag::Texture2DDesc{.format = PixelFormat::R8G8B8A8Unorm,
                                                  .width = 64,
                                                  .height = 64,
                                                  .sampleCount = 4,
                                                  .usage = TextureUsage::RenderTarget})
              .alignment == (4u << 20));

    // Undefined -> CopyDest -> ShaderRead for every texture.
    slag::CommandBuffer* commandBuffer =
        device->createCommandBuffer({.queueType = slag::CommandQueueType::Graphics}).value();
    REQUIRE(commandBuffer);
    REQUIRE(static_cast<bool>(commandBuffer->begin()));
    std::vector<slag::TextureBarrier> barriers;
    for (slag::Texture* texture : textures)
    {
        barriers.push_back({.syncBefore = slag::BarrierSync::None,
                            .syncAfter = slag::BarrierSync::Copy,
                            .accessBefore = slag::BarrierAccess::NoAccess,
                            .accessAfter = slag::BarrierAccess::CopyDest,
                            .layoutBefore = slag::TextureLayout::Undefined,
                            .layoutAfter = slag::TextureLayout::CopyDest,
                            .texture = texture});
    }
    commandBuffer->barrier({}, {}, barriers);
    for (slag::TextureBarrier& barrier : barriers)
    {
        barrier.syncBefore = slag::BarrierSync::Copy;
        barrier.syncAfter = slag::BarrierSync::PixelShading;
        barrier.accessBefore = slag::BarrierAccess::CopyDest;
        barrier.accessAfter = slag::BarrierAccess::ShaderRead;
        barrier.layoutBefore = slag::TextureLayout::CopyDest;
        barrier.layoutAfter = slag::TextureLayout::ShaderRead;
    }
    commandBuffer->barrier({}, {}, barriers);
    REQUIRE(static_cast<bool>(commandBuffer->end()));
    slag::Semaphore* semaphore = device->createSemaphore({}).value();
    slag::SemaphoreValue done = {semaphore, 1};
    slag::CommandBuffer* commandBuffers[] = {commandBuffer};
    REQUIRE(static_cast<bool>(device->commandQueue(slag::CommandQueueType::Graphics)
                                  ->submit({.commandBuffers = commandBuffers, .signals = {&done, 1}})));
    REQUIRE(static_cast<bool>(done.semaphore->wait(done.value, 5'000'000'000)));

    device->destroy(semaphore);
    device->destroy(commandBuffer);
    for (slag::Texture* texture : textures)
    {
        device->destroy(texture);
    }
    device->destroy(block);
    device->shutdown();
    test.checkNoErrors();
}

// Views of mips, layers, faces, and slices; castable formats; depth/stencil.
void testAttachmentViews(slag::Backend backend)
{
    TestInstance test(backend);
    slag::GraphicsDevice* device = test.firstDevice();
    REQUIRE(device);
    REQUIRE(static_cast<bool>(device->initialize({})));

    const PixelFormat srgb[] = {PixelFormat::R8G8B8A8UnormSrgb};
    slag::Texture* color = device
                               ->createTexture2D({.format = PixelFormat::R8G8B8A8Unorm,
                                                  .width = 256,
                                                  .height = 256,
                                                  .mipCount = 4,
                                                  .usage = TextureUsage::RenderTarget,
                                                  .castableFormats = srgb})
                               .value();
    slag::Texture* depth = device
                               ->createTexture2DArray({.format = PixelFormat::D32FloatS8Uint,
                                                       .width = 128,
                                                       .height = 128,
                                                       .layerCount = 4,
                                                       .usage = TextureUsage::DepthStencil})
                               .value();
    slag::Texture* msaa = device
                              ->createTexture2D({.format = PixelFormat::D32Float,
                                                 .width = 128,
                                                 .height = 128,
                                                 .sampleCount = 4,
                                                 .usage = TextureUsage::DepthStencil})
                              .value();
    slag::Texture* volume = device
                                ->createTexture3D({.format = PixelFormat::R16G16B16A16Float,
                                                   .width = 32,
                                                   .height = 32,
                                                   .depth = 8,
                                                   .usage = TextureUsage::RenderTarget})
                                .value();
    slag::Texture* cube =
        device
            ->createTextureCubeArray(
                {.format = PixelFormat::R8G8B8A8Unorm, .size = 64, .cubeCount = 2, .usage = TextureUsage::RenderTarget})
            .value();
    REQUIRE(color);
    REQUIRE(depth);
    REQUIRE(msaa);
    REQUIRE(volume);
    REQUIRE(cube);

    std::vector<slag::AttachmentView*> views;
    auto keep = [&](const slag::AttachmentViewDesc& desc) {
        slag::Result<slag::AttachmentView*> view = device->createAttachmentView(desc);
        REQUIRE(static_cast<bool>(view));
        views.push_back(view.value());
    };
    keep({.texture = color, .debugName = "mip 0"});
    keep({.texture = color, .format = PixelFormat::R8G8B8A8UnormSrgb, .mip = 3, .userData = 9});
    keep({.texture = depth, .baseLayer = 1, .layerCount = 3});
    keep({.texture = msaa});
    keep({.texture = volume, .baseLayer = 2, .layerCount = 4}); // depth slices
    keep({.texture = cube, .baseLayer = 6 + 4});                // second cube, +Z face
    keep({.texture = cube, .layerCount = 12});
    CHECK(views[1]->userData() == 9);

    for (slag::AttachmentView* view : views)
    {
        device->destroy(view);
    }
    for (slag::Texture* texture : {color, depth, msaa, volume, cube})
    {
        device->destroy(texture);
    }
    device->shutdown();
    test.checkNoErrors();
}

} // namespace

#ifdef SLAG_BACKEND_VULKAN
TEST_CASE("Vulkan: attachment views")
{
    testAttachmentViews(slag::Backend::Vulkan);
}
#endif

#ifdef SLAG_BACKEND_D3D12
TEST_CASE("D3D12: attachment views")
{
    testAttachmentViews(slag::Backend::D3D12);
}
#endif

#ifdef SLAG_BACKEND_VULKAN
TEST_CASE("Vulkan: every texture type, automatic and placed, with layout transitions")
{
    testTextureCreation(slag::Backend::Vulkan);
}
#endif

#ifdef SLAG_BACKEND_D3D12
TEST_CASE("D3D12: every texture type, automatic and placed, with layout transitions")
{
    testTextureCreation(slag::Backend::D3D12);
}
#endif
