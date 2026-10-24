// Native interop (ADR 0071): every accessor returns handles, and native
// commands recorded mid-stream mix with Slag's, which still find their
// descriptor heaps (and D3D12 root signature) bound afterwards.

#include "TestDevice.h"

#include <slag/Buffer.h>
#include <slag/CommandBuffer.h>
#include <slag/GraphicsDevice.h>
#include <slag/Pipeline.h>
#include <slag/QueryPool.h>
#include <slag/Semaphore.h>
#include <slag/Texture.h>

#ifdef SLAG_BACKEND_D3D12
#include <slag/native/D3D12.h>
#endif
#ifdef SLAG_BACKEND_VULKAN
#include <slag/native/Vulkan.h>
#endif

#include <cstdint>
#include <cstring>
#include <fstream>
#include <functional>
#include <iterator>
#include <string>
#include <vector>

namespace {

constexpr uint32_t count = 256;

std::vector<char> readFile(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

struct NativeScene
{
    slag::Pipeline* fill = nullptr;
    slag::Buffer* upload = nullptr;
    slag::Buffer* copied = nullptr; // written by the native copy
    slag::Buffer* filled = nullptr; // written by Slag's dispatch after it
    slag::Buffer* readback = nullptr;
    slag::MemoryBlock* block = nullptr;
    slag::Buffer* placed = nullptr;
    slag::Texture* texture = nullptr;
    slag::AttachmentView* view = nullptr;
    slag::QueryPool* pool = nullptr;
    slag::Semaphore* semaphore = nullptr;
    slag::CommandBuffer* commandBuffer = nullptr;
};

// Records: native copy upload -> copied; barrier; Slag dispatch writing
// `filled`; copies of both into the readback buffer.
void runScene(slag::Backend backend, const std::function<void(TestInstance&, NativeScene&)>& checkHandles,
              const std::function<void(NativeScene&)>& nativeCopy)
{
    TestInstance test(backend);
    slag::GraphicsDevice* device = test.firstDevice();
    REQUIRE(device);
    REQUIRE(static_cast<bool>(device->initialize({})));
    std::vector<char> spirv = readFile(SLAG_SHADER_DIR "/FillBuffer.main.spv");
    std::vector<char> dxil = readFile(SLAG_SHADER_DIR "/FillBuffer.main.dxil");
    std::vector<slag::ShaderBinary> binaries;
    if (!dxil.empty())
    {
        binaries.push_back({slag::ShaderFormat::Dxil, dxil.data(), dxil.size()});
    }
    if (!spirv.empty())
    {
        binaries.push_back({slag::ShaderFormat::Spirv, spirv.data(), spirv.size()});
    }
    NativeScene scene;
    scene.fill = device->createComputePipeline({.compute = {.binaries = binaries}}).value();
    auto buffer = [&](slag::MemoryKind kind) {
        return device->createBuffer({.size = count * 4, .placement = slag::Placement::automatic(kind)}).value();
    };
    scene.upload = buffer(slag::MemoryKind::Upload);
    scene.copied = buffer(slag::MemoryKind::GpuOnly);
    scene.filled = buffer(slag::MemoryKind::GpuOnly);
    scene.readback =
        device->createBuffer({.size = count * 8, .placement = slag::Placement::automatic(slag::MemoryKind::Readback)})
            .value();
    scene.block = device->createMemoryBlock({.size = 1 << 20}).value();
    scene.placed =
        device->createBuffer({.size = 256, .placement = slag::Placement::placed(scene.block, 65536)}).value();
    scene.texture = device
                        ->createTexture2D({.format = slag::PixelFormat::R8G8B8A8Unorm,
                                           .width = 16,
                                           .height = 16,
                                           .usage = slag::TextureUsage::RenderTarget})
                        .value();
    scene.view = device->createAttachmentView({.texture = scene.texture}).value();
    scene.pool = device->createQueryPool({.count = 1}).value();
    scene.semaphore = device->createSemaphore({}).value();
    scene.commandBuffer = device->createCommandBuffer({.queueType = slag::CommandQueueType::Compute}).value();
    REQUIRE((scene.fill && scene.upload && scene.copied && scene.filled && scene.readback && scene.placed &&
             scene.view && scene.pool && scene.semaphore && scene.commandBuffer));
    auto* words = static_cast<uint32_t*>(scene.upload->mappedData());
    for (uint32_t i = 0; i < count; ++i)
    {
        words[i] = 0xC0DE0000 + i;
    }
    device->writeRWStructuredBuffer(3, {.buffer = scene.filled, .count = count, .stride = 4});

    checkHandles(test, scene);

    REQUIRE(static_cast<bool>(scene.commandBuffer->begin()));
    nativeCopy(scene);
    slag::BufferBarrier afterCopy = {.syncBefore = slag::BarrierSync::Copy,
                                     .syncAfter = slag::BarrierSync::Copy,
                                     .accessBefore = slag::BarrierAccess::CopyDest,
                                     .accessAfter = slag::BarrierAccess::CopySource,
                                     .buffer = scene.copied};
    scene.commandBuffer->barrier({}, {&afterCopy, 1});
    scene.commandBuffer->bindPipeline(scene.fill);
    const uint32_t push[] = {3, count};
    scene.commandBuffer->pushConstants(push, sizeof(push));
    scene.commandBuffer->dispatch(count / 64, 1, 1);
    slag::BufferBarrier afterFill = {.syncBefore = slag::BarrierSync::ComputeShading,
                                     .syncAfter = slag::BarrierSync::Copy,
                                     .accessBefore = slag::BarrierAccess::ShaderWrite,
                                     .accessAfter = slag::BarrierAccess::CopySource,
                                     .buffer = scene.filled};
    scene.commandBuffer->barrier({}, {&afterFill, 1});
    scene.commandBuffer->copyBuffer(scene.readback, 0, scene.copied, 0, count * 4);
    scene.commandBuffer->copyBuffer(scene.readback, count * 4, scene.filled, 0, count * 4);
    REQUIRE(static_cast<bool>(scene.commandBuffer->end()));
    slag::SemaphoreValue done = {scene.semaphore, 1};
    slag::CommandBuffer* list[] = {scene.commandBuffer};
    REQUIRE(static_cast<bool>(device->commandQueue(slag::CommandQueueType::Compute)
                                  ->submit({.commandBuffers = list, .signals = {&done, 1}})));
    REQUIRE(static_cast<bool>(device->waitSemaphores({&done, 1}, slag::WaitMode::All, 5'000'000'000)));

    const auto* results = static_cast<const uint32_t*>(scene.readback->mappedData());
    uint32_t badCopies = 0;
    uint32_t badFills = 0;
    for (uint32_t i = 0; i < count; ++i)
    {
        badCopies += results[i] == 0xC0DE0000 + i ? 0 : 1;
        badFills += results[count + i] == i * 2 + 1 ? 0 : 1;
    }
    CHECK(badCopies == 0);
    CHECK(badFills == 0);

    device->destroy(scene.commandBuffer);
    device->destroy(scene.semaphore);
    device->destroy(scene.pool);
    device->destroy(scene.view);
    device->destroy(scene.texture);
    device->destroy(scene.placed);
    device->destroy(scene.block);
    device->destroy(scene.readback);
    device->destroy(scene.filled);
    device->destroy(scene.copied);
    device->destroy(scene.upload);
    device->destroy(scene.fill);
    device->shutdown();
    test.checkNoErrors();
}

} // namespace

#ifdef SLAG_BACKEND_VULKAN
TEST_CASE("Vulkan: native handles, and native commands mixed with Slag's")
{
    PFN_vkCmdCopyBuffer copyBuffer = nullptr;
    runScene(
        slag::Backend::Vulkan,
        [&](TestInstance& test, NativeScene& scene) {
            slag::GraphicsDevice* device = test.firstDevice();
            slag::vk::NativeDevice native = slag::vk::native(device);
            CHECK(native.instance == slag::vk::native(test.instance));
            CHECK(native.physicalDevice != VK_NULL_HANDLE);
            REQUIRE(native.device != VK_NULL_HANDLE);
            CHECK(slag::vk::native(device->commandQueue(slag::CommandQueueType::Compute)).queue != VK_NULL_HANDLE);
            CHECK(slag::vk::native(scene.commandBuffer) != VK_NULL_HANDLE);
            slag::vk::NativeBuffer placed = slag::vk::native(scene.placed);
            CHECK(placed.buffer != VK_NULL_HANDLE);
            CHECK(placed.memory == slag::vk::native(scene.block));
            CHECK(placed.offset == 65536);
            CHECK(slag::vk::native(scene.copied).memory != VK_NULL_HANDLE);
            CHECK(slag::vk::native(scene.texture).image != VK_NULL_HANDLE);
            CHECK(slag::vk::native(scene.view) != VK_NULL_HANDLE);
            CHECK(slag::vk::native(scene.fill) != VK_NULL_HANDLE);
            CHECK(slag::vk::native(scene.pool) != VK_NULL_HANDLE);
            CHECK(slag::vk::native(scene.semaphore) != VK_NULL_HANDLE);
            // The user loads their own entry points.
            PFN_vkGetInstanceProcAddr getInstanceProcAddr = slag::vk::getInstanceProcAddr(test.instance);
            REQUIRE(getInstanceProcAddr);
            auto getDeviceProcAddr =
                reinterpret_cast<PFN_vkGetDeviceProcAddr>(getInstanceProcAddr(native.instance, "vkGetDeviceProcAddr"));
            REQUIRE(getDeviceProcAddr);
            copyBuffer = reinterpret_cast<PFN_vkCmdCopyBuffer>(getDeviceProcAddr(native.device, "vkCmdCopyBuffer"));
            REQUIRE(copyBuffer);
        },
        [&](NativeScene& scene) {
            VkBufferCopy region = {0, 0, count * 4};
            copyBuffer(slag::vk::native(scene.commandBuffer), slag::vk::native(scene.upload).buffer,
                       slag::vk::native(scene.copied).buffer, 1, &region);
        });
}
#endif

#ifdef SLAG_BACKEND_D3D12
TEST_CASE("D3D12: native handles, and native commands mixed with Slag's")
{
    runScene(
        slag::Backend::D3D12,
        [](TestInstance& test, NativeScene& scene) {
            slag::GraphicsDevice* device = test.firstDevice();
            CHECK(slag::d3d12::native(test.instance) != nullptr);
            slag::d3d12::NativeDevice native = slag::d3d12::native(device);
            CHECK(native.device != nullptr);
            CHECK(native.adapter != nullptr);
            CHECK(native.resourceHeap != nullptr);
            CHECK(native.samplerHeap != nullptr);
            CHECK(native.rootSignature != nullptr);
            CHECK(slag::d3d12::native(device->commandQueue(slag::CommandQueueType::Compute)) != nullptr);
            CHECK(slag::d3d12::native(scene.commandBuffer) != nullptr);
            CHECK(slag::d3d12::native(scene.block) != nullptr);
            CHECK(slag::d3d12::native(scene.placed) != nullptr);
            CHECK(slag::d3d12::native(scene.texture) != nullptr);
            CHECK(slag::d3d12::native(scene.view).ptr != 0);
            CHECK(slag::d3d12::native(scene.fill) != nullptr);
            CHECK(slag::d3d12::native(scene.pool) != nullptr);
            CHECK(slag::d3d12::native(scene.semaphore) != nullptr);
        },
        [](NativeScene& scene) {
            slag::d3d12::native(scene.commandBuffer)
                ->CopyBufferRegion(slag::d3d12::native(scene.copied), 0, slag::d3d12::native(scene.upload), 0,
                                   count * 4);
        });
}
#endif
