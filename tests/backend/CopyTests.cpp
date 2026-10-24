#include "TestDevice.h"

#include <slag/Buffer.h>
#include <slag/CommandBuffer.h>
#include <slag/GraphicsDevice.h>
#include <slag/Limits.h>
#include <slag/Semaphore.h>
#include <slag/Texture.h>

#include <cstdint>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

namespace {

using slag::PixelFormat;
using slag::TexturePlane;

struct TextureCase
{
    std::string name;
    std::function<slag::Result<slag::Texture*>(slag::GraphicsDevice*)> create;
    uint32_t mipCount;
    uint32_t layerCount; // 1 for 3D
    std::vector<TexturePlane> planes;
    uint32_t blockBytes;         // per texel or 4x4 block; the stencil plane is 1
    uint32_t blockDimension = 1; // 4 when block-compressed
    bool depthValues = false;    // the depth plane holds floats in [0, 1]
};

struct Subresource
{
    uint32_t mip;
    uint32_t layer;
    TexturePlane plane;
    slag::CopyFootprint footprint;
    uint64_t offset;   // in the staging buffers
    uint64_t rowBytes; // texel bytes per row; the rest of the pitch isn't copied
};

uint64_t alignUp(uint64_t value, uint64_t alignment)
{
    return (value + alignment - 1) / alignment * alignment;
}

slag::TextureBarrier transition(slag::Texture* texture, slag::TextureLayout before, slag::TextureLayout after)
{
    auto access = [](slag::TextureLayout layout) {
        return layout == slag::TextureLayout::CopyDest     ? slag::BarrierAccess::CopyDest
               : layout == slag::TextureLayout::CopySource ? slag::BarrierAccess::CopySource
                                                           : slag::BarrierAccess::NoAccess;
    };
    return {.syncBefore = before == slag::TextureLayout::Undefined ? slag::BarrierSync::None : slag::BarrierSync::Copy,
            .syncAfter = slag::BarrierSync::Copy,
            .accessBefore = access(before),
            .accessAfter = access(after),
            .layoutBefore = before,
            .layoutAfter = after,
            .texture = texture};
}

// Upload buffer -> texture A -> texture B -> readback buffer, for every mip,
// layer, and plane; the readback must match the upload byte for byte.
void roundTrip(TestInstance& test, slag::GraphicsDevice* device, const TextureCase& testCase)
{
    CAPTURE(testCase.name);
    slag::Result<slag::Texture*> a = testCase.create(device);
    slag::Result<slag::Texture*> b = testCase.create(device);
    REQUIRE(static_cast<bool>(a));
    REQUIRE(static_cast<bool>(b));

    std::vector<Subresource> subresources;
    uint64_t total = 0;
    for (uint32_t mip = 0; mip < testCase.mipCount; ++mip)
    {
        for (uint32_t layer = 0; layer < testCase.layerCount; ++layer)
        {
            for (TexturePlane plane : testCase.planes)
            {
                slag::CopyFootprint footprint = device->copyFootprint(a.value(), mip, plane);
                REQUIRE(footprint.size > 0);
                CHECK(footprint.rowPitch % slag::alignment::textureCopyRowPitch == 0);
                total = alignUp(total, slag::alignment::textureCopyOffset);
                uint32_t blocksWide = (footprint.width + testCase.blockDimension - 1) / testCase.blockDimension;
                uint32_t bytes = plane == TexturePlane::Stencil ? 1 : testCase.blockBytes;
                subresources.push_back({mip, layer, plane, footprint, total, uint64_t(blocksWide) * bytes});
                total += footprint.size;
            }
        }
    }

    slag::Buffer* upload =
        device->createBuffer({.size = total, .placement = slag::Placement::automatic(slag::MemoryKind::Upload)})
            .value();
    slag::Buffer* readback =
        device->createBuffer({.size = total, .placement = slag::Placement::automatic(slag::MemoryKind::Readback)})
            .value();
    REQUIRE(upload);
    REQUIRE(readback);
    auto* bytes = static_cast<uint8_t*>(upload->mappedData());
    for (const Subresource& sub : subresources)
    {
        uint8_t* begin = bytes + sub.offset;
        if (testCase.depthValues && sub.plane == TexturePlane::Depth)
        {
            auto* depths = reinterpret_cast<float*>(begin);
            for (uint64_t i = 0; i < sub.footprint.size / 4; ++i)
            {
                depths[i] = static_cast<float>((i * 37 + sub.mip * 11 + sub.layer) % 1024) / 1023.0f;
            }
        }
        else
        {
            for (uint64_t i = 0; i < sub.footprint.size; ++i)
            {
                begin[i] = static_cast<uint8_t>(i * 13 + sub.mip * 101 + sub.layer * 59 + 1);
            }
        }
    }
    std::memset(readback->mappedData(), 0, total);

    slag::CommandBuffer* commandBuffer =
        device->createCommandBuffer({.queueType = slag::CommandQueueType::Graphics}).value();
    REQUIRE(commandBuffer);
    REQUIRE(static_cast<bool>(commandBuffer->begin()));
    slag::TextureBarrier toDest[] = {
        transition(a.value(), slag::TextureLayout::Undefined, slag::TextureLayout::CopyDest),
        transition(b.value(), slag::TextureLayout::Undefined, slag::TextureLayout::CopyDest)};
    commandBuffer->barrier({}, {}, toDest);
    for (const Subresource& sub : subresources)
    {
        slag::TextureLocation location = {.texture = a.value(), .mip = sub.mip, .layer = sub.layer, .plane = sub.plane};
        commandBuffer->copyBufferToTexture(location, {upload, sub.offset, sub.footprint.rowPitch}, sub.footprint.width,
                                           sub.footprint.height, sub.footprint.depth);
    }
    slag::TextureBarrier aToSource =
        transition(a.value(), slag::TextureLayout::CopyDest, slag::TextureLayout::CopySource);
    commandBuffer->barrier({}, {}, {&aToSource, 1});
    for (const Subresource& sub : subresources)
    {
        commandBuffer->copyTexture({.texture = b.value(), .mip = sub.mip, .layer = sub.layer, .plane = sub.plane},
                                   {.texture = a.value(), .mip = sub.mip, .layer = sub.layer, .plane = sub.plane},
                                   sub.footprint.width, sub.footprint.height, sub.footprint.depth);
    }
    slag::TextureBarrier bToSource =
        transition(b.value(), slag::TextureLayout::CopyDest, slag::TextureLayout::CopySource);
    commandBuffer->barrier({}, {}, {&bToSource, 1});
    for (const Subresource& sub : subresources)
    {
        commandBuffer->copyTextureToBuffer(
            {readback, sub.offset, sub.footprint.rowPitch},
            {.texture = b.value(), .mip = sub.mip, .layer = sub.layer, .plane = sub.plane}, sub.footprint.width,
            sub.footprint.height, sub.footprint.depth);
    }
    REQUIRE(static_cast<bool>(commandBuffer->end()));

    slag::Semaphore* semaphore = device->createSemaphore({}).value();
    slag::SemaphoreValue done = {semaphore, 1};
    slag::CommandBuffer* commandBuffers[] = {commandBuffer};
    REQUIRE(static_cast<bool>(device->commandQueue(slag::CommandQueueType::Graphics)
                                  ->submit({.commandBuffers = commandBuffers, .signals = {&done, 1}})));
    REQUIRE(static_cast<bool>(done.semaphore->wait(done.value, 10'000'000'000)));

    // Compare the texel bytes of every row.
    const auto* expected = static_cast<const uint8_t*>(upload->mappedData());
    const auto* actual = static_cast<const uint8_t*>(readback->mappedData());
    uint32_t mismatched = 0;
    for (const Subresource& sub : subresources)
    {
        uint64_t rows = uint64_t(sub.footprint.rowCount) * sub.footprint.depth;
        for (uint64_t row = 0; row < rows; ++row)
        {
            uint64_t begin = sub.offset + row * sub.footprint.rowPitch;
            if (std::memcmp(expected + begin, actual + begin, sub.rowBytes) != 0)
            {
                ++mismatched;
            }
        }
    }
    CHECK(mismatched == 0);

    device->destroy(semaphore);
    device->destroy(commandBuffer);
    device->destroy(upload);
    device->destroy(readback);
    device->destroy(a.value());
    device->destroy(b.value());
    test.checkNoErrors();
}

std::vector<TextureCase> textureCases()
{
    using slag::TextureUsage;
    std::vector<TextureCase> cases;
    cases.push_back({"1D",
                     [](slag::GraphicsDevice* d) {
                         return d->createTexture1D({.format = PixelFormat::R8G8B8A8Unorm, .width = 100, .mipCount = 2});
                     },
                     2,
                     1,
                     {TexturePlane::Color},
                     4});
    cases.push_back({"1D array",
                     [](slag::GraphicsDevice* d) {
                         return d->createTexture1DArray(
                             {.format = PixelFormat::R16G16B16A16Float, .width = 50, .layerCount = 3});
                     },
                     1,
                     3,
                     {TexturePlane::Color},
                     8});
    cases.push_back({"2D",
                     [](slag::GraphicsDevice* d) {
                         return d->createTexture2D(
                             {.format = PixelFormat::R8G8B8A8Unorm, .width = 64, .height = 48, .mipCount = 3});
                     },
                     3,
                     1,
                     {TexturePlane::Color},
                     4});
    cases.push_back(
        {"2D array",
         [](slag::GraphicsDevice* d) {
             return d->createTexture2DArray(
                 {.format = PixelFormat::R32Float, .width = 32, .height = 32, .layerCount = 4, .mipCount = 2});
         },
         2,
         4,
         {TexturePlane::Color},
         4});
    cases.push_back({"3D",
                     [](slag::GraphicsDevice* d) {
                         return d->createTexture3D(
                             {.format = PixelFormat::R8G8Unorm, .width = 16, .height = 8, .depth = 4, .mipCount = 3});
                     },
                     3,
                     1,
                     {TexturePlane::Color},
                     2});
    cases.push_back({"cube",
                     [](slag::GraphicsDevice* d) {
                         return d->createTextureCube({.format = PixelFormat::R8G8B8A8Unorm, .size = 16, .mipCount = 2});
                     },
                     2,
                     6,
                     {TexturePlane::Color},
                     4});
    cases.push_back({"cube array",
                     [](slag::GraphicsDevice* d) {
                         return d->createTextureCubeArray(
                             {.format = PixelFormat::R10G10B10A2Unorm, .size = 8, .cubeCount = 2});
                     },
                     1,
                     12,
                     {TexturePlane::Color},
                     4});
    cases.push_back({"BC7",
                     [](slag::GraphicsDevice* d) {
                         return d->createTexture2D(
                             {.format = PixelFormat::BC7Unorm, .width = 16, .height = 16, .mipCount = 3});
                     },
                     3,
                     1,
                     {TexturePlane::Color},
                     16,
                     4});
    cases.push_back({"depth/stencil",
                     [](slag::GraphicsDevice* d) {
                         return d->createTexture2D({.format = PixelFormat::D32FloatS8Uint,
                                                    .width = 32,
                                                    .height = 32,
                                                    .usage = TextureUsage::DepthStencil});
                     },
                     1,
                     1,
                     {TexturePlane::Depth, TexturePlane::Stencil},
                     4,
                     1,
                     true});
    cases.push_back({"depth 16",
                     [](slag::GraphicsDevice* d) {
                         return d->createTexture2D({.format = PixelFormat::D16Unorm, .width = 24, .height = 24});
                     },
                     1,
                     1,
                     {TexturePlane::Color},
                     2});
    return cases;
}

void testRoundTrips(slag::Backend backend)
{
    TestInstance test(backend);
    for (slag::GraphicsDevice* device : test.instance->graphicsDevices())
    {
        MESSAGE("device: " << std::string(device->info().name));
        REQUIRE(static_cast<bool>(device->initialize({})));
        for (const TextureCase& testCase : textureCases())
        {
            roundTrip(test, device, testCase);
        }
        device->shutdown();
    }
}

} // namespace

#ifdef SLAG_BACKEND_VULKAN
TEST_CASE("Vulkan: every texture type round-trips buffer -> texture -> texture -> buffer")
{
    testRoundTrips(slag::Backend::Vulkan);
}
#endif

#ifdef SLAG_BACKEND_D3D12
TEST_CASE("D3D12: every texture type round-trips buffer -> texture -> texture -> buffer")
{
    testRoundTrips(slag::Backend::D3D12);
}
#endif
