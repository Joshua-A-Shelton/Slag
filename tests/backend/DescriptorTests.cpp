#include "TestDevice.h"

#include <slag/Buffer.h>
#include <slag/CommandBuffer.h>
#include <slag/Descriptors.h>
#include <slag/GraphicsDevice.h>
#include <slag/Pipeline.h>
#include <slag/Semaphore.h>
#include <slag/Texture.h>

#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace {

std::vector<char> readFile(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

slag::Pipeline* computePipeline(slag::GraphicsDevice* device, const char* name, std::vector<char>& spirv,
                                std::vector<char>& dxil)
{
    spirv = readFile(std::string(SLAG_SHADER_DIR "/") + name + ".main.spv");
    dxil = readFile(std::string(SLAG_SHADER_DIR "/") + name + ".main.dxil");
    std::vector<slag::ShaderBinary> binaries;
    if (!dxil.empty())
    {
        binaries.push_back({slag::ShaderFormat::Dxil, dxil.data(), dxil.size()});
    }
    if (!spirv.empty())
    {
        binaries.push_back({slag::ShaderFormat::Spirv, spirv.data(), spirv.size()});
    }
    return device->createComputePipeline({.compute = {.binaries = binaries}, .debugName = name}).value();
}

struct Float4
{
    float x, y, z, w;
};

bool equal(const Float4& a, const Float4& b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

// One value through every descriptor kind (DescriptorKinds.slang).
void testDescriptorKinds(slag::Backend backend)
{
    TestInstance test(backend);
    slag::GraphicsDevice* device = test.firstDevice();
    REQUIRE(device);
    REQUIRE(static_cast<bool>(device->initialize({})));
    std::vector<char> spirv;
    std::vector<char> dxil;
    slag::Pipeline* pipeline = computePipeline(device, "DescriptorKinds", spirv, dxil);
    REQUIRE(pipeline);

    // Source data in one upload buffer: constants at 0, raw at 256, structured
    // at 512, typed at 768, texture texels at 1024 (4x4 RGBA8, 256-byte rows).
    slag::Buffer* upload =
        device->createBuffer({.size = 8192, .placement = slag::Placement::automatic(slag::MemoryKind::Upload)}).value();
    slag::Buffer* output =
        device->createBuffer({.size = 6 * 16, .placement = slag::Placement::automatic(slag::MemoryKind::GpuOnly)})
            .value();
    slag::Buffer* readback =
        device->createBuffer({.size = 4096, .placement = slag::Placement::automatic(slag::MemoryKind::Readback)})
            .value();
    REQUIRE(upload);
    REQUIRE(output);
    REQUIRE(readback);
    auto* bytes = static_cast<uint8_t*>(upload->mappedData());
    std::memset(bytes, 0, 8192);
    const Float4 constants = {1.0f, 2.0f, 3.0f, 4.0f};
    const Float4 raw = {5.0f, 6.0f, 7.0f, 8.0f};           // at raw offset 16
    const Float4 structured = {9.0f, 10.0f, 11.0f, 12.0f}; // element 1
    const Float4 typed = {13.0f, 14.0f, 15.0f, 16.0f};     // element 2
    std::memcpy(bytes + 0, &constants, 16);
    std::memcpy(bytes + 256 + 16, &raw, 16);
    std::memcpy(bytes + 512 + 16, &structured, 16);
    std::memcpy(bytes + 768 + 32, &typed, 16);
    for (uint32_t y = 0; y < 4; ++y)
    {
        for (uint32_t x = 0; x < 4; ++x)
        {
            uint8_t* texel = bytes + 1024 + y * 256 + x * 4;
            texel[0] = static_cast<uint8_t>(x * 85); // 0, 85, 170, 255: exact in unorm
            texel[1] = static_cast<uint8_t>(y * 85);
            texel[2] = 255;
            texel[3] = 255;
        }
    }

    slag::Texture* texture =
        device->createTexture2D({.format = slag::PixelFormat::R8G8B8A8Unorm, .width = 4, .height = 4}).value();
    slag::Texture* rwTexture = device
                                   ->createTexture2D({.format = slag::PixelFormat::R8G8B8A8Unorm,
                                                      .width = 4,
                                                      .height = 4,
                                                      .usage = slag::TextureUsage::ShaderWrite})
                                   .value();
    REQUIRE(texture);
    REQUIRE(rwTexture);

    enum Slot : uint32_t
    {
        ConstantsSlot = 10,
        RawSlot,
        StructuredSlot,
        TypedSlot,
        TextureSlot,
        RWTextureSlot,
        OutputSlot,
        SamplerSlot = 3,
    };
    device->writeConstantBuffer(ConstantsSlot, {.address = upload->gpuAddress(), .size = 256});
    device->writeRawBuffer(RawSlot, {.buffer = upload, .offset = 256, .size = 256});
    const slag::StructuredBufferDescriptorWrite structuredWrites[] = {
        {StructuredSlot, {.buffer = upload, .offset = 512, .count = 4, .stride = 16}},
        {OutputSlot, {.buffer = output, .offset = 0, .count = 6, .stride = 16}}};
    device->writeStructuredBuffer({&structuredWrites[0], 1});
    device->writeRWStructuredBuffer(OutputSlot, structuredWrites[1].descriptor);
    device->writeTypedBuffer(
        TypedSlot, {.buffer = upload, .offset = 768, .count = 4, .format = slag::PixelFormat::R32G32B32A32Float});
    device->writeTexture(TextureSlot, {.texture = texture});
    device->writeRWTexture(RWTextureSlot, {.texture = rwTexture});
    device->writeSampler(SamplerSlot, {.minFilter = slag::Filter::Point,
                                       .magFilter = slag::Filter::Point,
                                       .mipFilter = slag::Filter::Point,
                                       .addressU = slag::AddressMode::Clamp,
                                       .addressV = slag::AddressMode::Clamp});

    slag::CommandBuffer* commandBuffer =
        device->createCommandBuffer({.queueType = slag::CommandQueueType::Graphics}).value();
    REQUIRE(commandBuffer);
    REQUIRE(static_cast<bool>(commandBuffer->begin()));
    slag::TextureBarrier toCopy[] = {{.syncBefore = slag::BarrierSync::None,
                                      .syncAfter = slag::BarrierSync::Copy,
                                      .accessAfter = slag::BarrierAccess::CopyDest,
                                      .layoutBefore = slag::TextureLayout::Undefined,
                                      .layoutAfter = slag::TextureLayout::CopyDest,
                                      .texture = texture},
                                     {.syncBefore = slag::BarrierSync::None,
                                      .syncAfter = slag::BarrierSync::ComputeShading,
                                      .accessAfter = slag::BarrierAccess::ShaderWrite,
                                      .layoutBefore = slag::TextureLayout::Undefined,
                                      .layoutAfter = slag::TextureLayout::ShaderWrite,
                                      .texture = rwTexture}};
    commandBuffer->barrier({}, {}, toCopy);
    commandBuffer->copyBufferToTexture({.texture = texture}, {.buffer = upload, .offset = 1024, .rowPitch = 256}, 4, 4);
    slag::TextureBarrier toRead = {.syncBefore = slag::BarrierSync::Copy,
                                   .syncAfter = slag::BarrierSync::ComputeShading,
                                   .accessBefore = slag::BarrierAccess::CopyDest,
                                   .accessAfter = slag::BarrierAccess::ShaderRead,
                                   .layoutBefore = slag::TextureLayout::CopyDest,
                                   .layoutAfter = slag::TextureLayout::ShaderRead,
                                   .texture = texture};
    commandBuffer->barrier({}, {}, {&toRead, 1});
    commandBuffer->bindPipeline(pipeline);
    const uint32_t push[] = {ConstantsSlot, RawSlot,     StructuredSlot, TypedSlot,
                             TextureSlot,   SamplerSlot, RWTextureSlot,  OutputSlot};
    commandBuffer->pushConstants(push, sizeof(push));
    commandBuffer->dispatch(1, 1, 1);
    slag::BufferBarrier outputToCopy = {.syncBefore = slag::BarrierSync::ComputeShading,
                                        .syncAfter = slag::BarrierSync::Copy,
                                        .accessBefore = slag::BarrierAccess::ShaderWrite,
                                        .accessAfter = slag::BarrierAccess::CopySource,
                                        .buffer = output};
    slag::TextureBarrier rwToCopy = {.syncBefore = slag::BarrierSync::ComputeShading,
                                     .syncAfter = slag::BarrierSync::Copy,
                                     .accessBefore = slag::BarrierAccess::ShaderWrite,
                                     .accessAfter = slag::BarrierAccess::CopySource,
                                     .layoutBefore = slag::TextureLayout::ShaderWrite,
                                     .layoutAfter = slag::TextureLayout::CopySource,
                                     .texture = rwTexture};
    commandBuffer->barrier({}, {&outputToCopy, 1}, {&rwToCopy, 1});
    commandBuffer->copyBuffer(readback, 0, output, 0, 6 * 16);
    commandBuffer->copyTextureToBuffer({.buffer = readback, .offset = 1024, .rowPitch = 256}, {.texture = rwTexture}, 4,
                                       4);
    REQUIRE(static_cast<bool>(commandBuffer->end()));

    slag::Semaphore* semaphore = device->createSemaphore({}).value();
    slag::SemaphoreValue done = {semaphore, 1};
    slag::CommandBuffer* commandBuffers[] = {commandBuffer};
    REQUIRE(static_cast<bool>(device->commandQueue(slag::CommandQueueType::Graphics)
                                  ->submit({.commandBuffers = commandBuffers, .signals = {&done, 1}})));
    REQUIRE(static_cast<bool>(device->waitSemaphores({&done, 1}, slag::WaitMode::All, 5'000'000'000)));

    const auto* results = static_cast<const Float4*>(readback->mappedData());
    CHECK(equal(results[0], constants));
    CHECK(equal(results[1], raw));
    CHECK(equal(results[2], structured));
    CHECK(equal(results[3], typed));
    CHECK(equal(results[4], {170.0f / 255.0f, 85.0f / 255.0f, 1.0f, 1.0f}));                    // sampled texel (2, 1)
    CHECK(equal(results[5], {255.0f / 255.0f, 170.0f / 255.0f, 1.0f, 1.0f}));                   // loaded texel (3, 2)
    const auto* written = static_cast<const uint8_t*>(readback->mappedData()) + 1024 + 256 + 4; // texel (1, 1)
    CHECK(written[0] == 255);
    CHECK(written[1] == 0);
    CHECK(written[2] == 255);
    CHECK(written[3] == 255);

    device->destroy(semaphore);
    device->destroy(commandBuffer);
    device->destroy(texture);
    device->destroy(rwTexture);
    device->destroy(upload);
    device->destroy(output);
    device->destroy(readback);
    device->destroy(pipeline);
    device->shutdown();
    test.checkNoErrors();
}

// setConstantBuffer: two buffers set in turn, each read by one dispatch.
void testRootConstantBuffer(slag::Backend backend)
{
    TestInstance test(backend);
    slag::GraphicsDevice* device = test.firstDevice();
    REQUIRE(device);
    REQUIRE(static_cast<bool>(device->initialize({})));
    std::vector<char> spirv;
    std::vector<char> dxil;
    slag::Pipeline* pipeline = computePipeline(device, "RootConstants", spirv, dxil);
    REQUIRE(pipeline);

    // Two constant blocks at 0 and 256; two outputs.
    slag::Buffer* constants =
        device->createBuffer({.size = 512, .placement = slag::Placement::automatic(slag::MemoryKind::Upload)}).value();
    // GpuOnly: D3D12 readback memory can't be written by shaders.
    slag::Buffer* output =
        device->createBuffer({.size = 64, .placement = slag::Placement::automatic(slag::MemoryKind::GpuOnly)}).value();
    slag::Buffer* readback =
        device->createBuffer({.size = 64, .placement = slag::Placement::automatic(slag::MemoryKind::Readback)}).value();
    REQUIRE(constants);
    REQUIRE(output);
    REQUIRE(readback);
    auto* bytes = static_cast<uint8_t*>(constants->mappedData());
    const Float4 first[2] = {{1.0f, 2.0f, 3.0f, 4.0f}, {5.0f, 6.0f, 7.0f, 8.0f}};
    const uint32_t second[2][4] = {{10, 20, 30, 40}, {50, 60, 70, 80}};
    for (int i = 0; i < 2; ++i)
    {
        std::memcpy(bytes + i * 256, &first[i], 16);
        std::memcpy(bytes + i * 256 + 16, second[i], 16);
    }
    // The outputs are slots 20 and 21, 32 bytes each.
    device->writeRWStructuredBuffer(20, {.buffer = output, .offset = 0, .count = 2, .stride = 16});
    device->writeRWStructuredBuffer(21, {.buffer = output, .offset = 32, .count = 2, .stride = 16});

    slag::CommandBuffer* commandBuffer =
        device->createCommandBuffer({.queueType = slag::CommandQueueType::Compute}).value();
    REQUIRE(static_cast<bool>(commandBuffer->begin()));
    commandBuffer->bindPipeline(pipeline);
    for (uint32_t i = 0; i < 2; ++i)
    {
        uint32_t slot = 20 + i;
        commandBuffer->pushConstants(&slot, sizeof(slot));
        commandBuffer->setConstantBuffer(constants->gpuAddress() + i * 256, 256);
        commandBuffer->dispatch(1, 1, 1);
    }
    slag::BufferBarrier toCopy = {.syncBefore = slag::BarrierSync::ComputeShading,
                                  .syncAfter = slag::BarrierSync::Copy,
                                  .accessBefore = slag::BarrierAccess::ShaderWrite,
                                  .accessAfter = slag::BarrierAccess::CopySource,
                                  .buffer = output};
    commandBuffer->barrier({}, {&toCopy, 1});
    commandBuffer->copyBuffer(readback, 0, output, 0, 64);
    REQUIRE(static_cast<bool>(commandBuffer->end()));
    slag::Semaphore* semaphore = device->createSemaphore({}).value();
    slag::SemaphoreValue done = {semaphore, 1};
    slag::CommandBuffer* commandBuffers[] = {commandBuffer};
    REQUIRE(static_cast<bool>(device->commandQueue(slag::CommandQueueType::Compute)
                                  ->submit({.commandBuffers = commandBuffers, .signals = {&done, 1}})));
    REQUIRE(static_cast<bool>(device->waitSemaphores({&done, 1}, slag::WaitMode::All, 5'000'000'000)));

    const auto* results = static_cast<const uint8_t*>(readback->mappedData());
    for (int i = 0; i < 2; ++i)
    {
        CAPTURE(i);
        Float4 value;
        uint32_t integers[4];
        std::memcpy(&value, results + i * 32, 16);
        std::memcpy(integers, results + i * 32 + 16, 16);
        CHECK(equal(value, first[i]));
        CHECK(std::memcmp(integers, second[i], 16) == 0);
    }

    device->destroy(semaphore);
    device->destroy(commandBuffer);
    device->destroy(constants);
    device->destroy(output);
    device->destroy(readback);
    device->destroy(pipeline);
    device->shutdown();
    test.checkNoErrors();
}

#ifdef SLAG_VALIDATION
// Slot tracking (ADR 0031): destroying a resource still in slots logs one
// Info message; clearing or overwriting the slot first doesn't. On D3D12 the
// slots then read as poison (ADR 0016): 0xDEADBEEF for buffers and integer
// textures, magenta for float textures.
void testReleasedSlots(slag::Backend backend)
{
    TestInstance test(backend);
    slag::GraphicsDevice* device = test.firstDevice();
    REQUIRE(device);
    REQUIRE(static_cast<bool>(device->initialize({})));
    std::vector<char> spirv;
    std::vector<char> dxil;
    slag::Pipeline* pipeline = computePipeline(device, "ReadPoison", spirv, dxil);
    REQUIRE(pipeline);
    auto gpuBuffer = [&](uint64_t size, const char* name) {
        return device
            ->createBuffer(
                {.size = size, .placement = slag::Placement::automatic(slag::MemoryKind::GpuOnly), .debugName = name})
            .value();
    };
    auto texture = [&](slag::PixelFormat format, const char* name) {
        return device->createTexture2D({.format = format, .width = 4, .height = 4, .debugName = name}).value();
    };
    slag::Buffer* raw = gpuBuffer(256, "raw");
    slag::Buffer* overwritten = gpuBuffer(256, "overwritten");
    slag::Texture* floats = texture(slag::PixelFormat::R8G8B8A8Unorm, "floats");
    slag::Texture* uints = texture(slag::PixelFormat::R32G32B32A32Uint, "uints");
    slag::Buffer* output = gpuBuffer(64, "output");
    slag::Buffer* readback =
        device->createBuffer({.size = 64, .placement = slag::Placement::automatic(slag::MemoryKind::Readback)}).value();
    REQUIRE((raw && overwritten && floats && uints && output && readback));

    device->writeRawBuffer(1, {.buffer = raw, .size = 256});
    device->writeRawBuffer(5, {.buffer = raw, .size = 256}); // two slots, one message
    device->writeTexture(2, {.texture = floats});
    device->writeTexture(3, {.texture = uints});
    device->writeRWStructuredBuffer(4, {.buffer = output, .count = 16, .stride = 4});
    device->writeRawBuffer(6, {.buffer = overwritten, .size = 256});
    device->writeRawBuffer(6, {.buffer = raw, .size = 256}); // no longer holds `overwritten`
    device->clearResourceDescriptor(2);

    test.messages.slagInfo.clear();
    device->destroy(overwritten);
    device->destroy(floats);
    CHECK(test.messages.slagInfo.empty()); // neither is in a slot any more
    device->destroy(raw);
    device->destroy(uints);
    REQUIRE(test.messages.slagInfo.size() == 2);
    CHECK(test.messages.slagInfo[0].find("buffer \"raw\" was still in 3 resource slots (first: 1)") !=
          std::string::npos);
    CHECK(test.messages.slagInfo[1].find("texture \"uints\" was still in 1 resource slots") != std::string::npos);

    if (backend == slag::Backend::D3D12)
    {
        slag::CommandBuffer* commandBuffer =
            device->createCommandBuffer({.queueType = slag::CommandQueueType::Compute}).value();
        REQUIRE(static_cast<bool>(commandBuffer->begin()));
        commandBuffer->bindPipeline(pipeline);
        const uint32_t push[] = {1, 2, 3, 4};
        commandBuffer->pushConstants(push, sizeof(push));
        commandBuffer->dispatch(1, 1, 1);
        slag::BufferBarrier toCopy = {.syncBefore = slag::BarrierSync::ComputeShading,
                                      .syncAfter = slag::BarrierSync::Copy,
                                      .accessBefore = slag::BarrierAccess::ShaderWrite,
                                      .accessAfter = slag::BarrierAccess::CopySource,
                                      .buffer = output};
        commandBuffer->barrier({}, {&toCopy, 1});
        commandBuffer->copyBuffer(readback, 0, output, 0, 64);
        REQUIRE(static_cast<bool>(commandBuffer->end()));
        slag::Semaphore* semaphore = device->createSemaphore({}).value();
        slag::SemaphoreValue done = {semaphore, 1};
        slag::CommandBuffer* commandBuffers[] = {commandBuffer};
        REQUIRE(static_cast<bool>(device->commandQueue(slag::CommandQueueType::Compute)
                                      ->submit({.commandBuffers = commandBuffers, .signals = {&done, 1}})));
        REQUIRE(static_cast<bool>(device->waitSemaphores({&done, 1}, slag::WaitMode::All, 5'000'000'000)));
        uint32_t values[7] = {};
        std::memcpy(values, readback->mappedData(), sizeof(values));
        float magenta[4] = {};
        std::memcpy(magenta, values + 1, sizeof(magenta));
        CHECK(values[0] == 0xDEADBEEF);
        CHECK(magenta[0] == 1.0f);
        CHECK(magenta[1] == 0.0f);
        CHECK(magenta[2] == 1.0f);
        CHECK(magenta[3] == 1.0f);
        CHECK(values[5] == 0xDEADBEEF);
        CHECK(values[6] == 0xDEADBEEF);
        device->destroy(semaphore);
        device->destroy(commandBuffer);
    }

    device->destroy(output);
    device->destroy(readback);
    device->destroy(pipeline);
    device->shutdown();
    test.checkNoErrors();
}
#endif

} // namespace

#ifdef SLAG_BACKEND_VULKAN
TEST_CASE("Vulkan: the root constant buffer slot")
{
    testRootConstantBuffer(slag::Backend::Vulkan);
}
#endif

#ifdef SLAG_BACKEND_D3D12
TEST_CASE("D3D12: the root constant buffer slot")
{
    testRootConstantBuffer(slag::Backend::D3D12);
}
#endif

#ifdef SLAG_BACKEND_VULKAN
TEST_CASE("Vulkan: every descriptor kind reads and writes through the heaps")
{
    testDescriptorKinds(slag::Backend::Vulkan);
}
#endif

#ifdef SLAG_BACKEND_D3D12
TEST_CASE("D3D12: every descriptor kind reads and writes through the heaps")
{
    testDescriptorKinds(slag::Backend::D3D12);
}
#endif

#ifdef SLAG_VALIDATION
#ifdef SLAG_BACKEND_VULKAN
TEST_CASE("Vulkan: destroying a resource still in slots is reported once")
{
    testReleasedSlots(slag::Backend::Vulkan);
}
#endif

#ifdef SLAG_BACKEND_D3D12
TEST_CASE("D3D12: slots of destroyed and cleared resources read as poison")
{
    testReleasedSlots(slag::Backend::D3D12);
}
#endif
#endif

#ifdef SLAG_BACKEND_D3D12
// Open item 12 (settled in M7): D3D12 GPU-based validation reports reads of
// descriptors that were never written, one error per slot.
TEST_CASE("D3D12: GPU-based validation reports never-written descriptors")
{
    TestMessages messages;
    slag::InstanceDesc desc;
    desc.backend = slag::Backend::D3D12;
    desc.validationFlags = slag::ValidationFlags::Api | slag::ValidationFlags::GpuAssisted;
    desc.validationFailureAction = slag::ValidationFailureAction::Continue;
    desc.allowSoftwareDevices = true;
    desc.messageCallback = collectTestMessage;
    desc.messageUserData = &messages;
    slag::Instance* instance = slag::createInstance(desc).value();
    REQUIRE(instance);
    slag::GraphicsDevice* device = instance->graphicsDevices()[0];
    REQUIRE(static_cast<bool>(device->initialize({})));
    std::vector<char> spirv;
    std::vector<char> dxil;
    slag::Pipeline* pipeline = computePipeline(device, "ReadPoison", spirv, dxil);
    slag::Buffer* output =
        device->createBuffer({.size = 64, .placement = slag::Placement::automatic(slag::MemoryKind::GpuOnly)}).value();
    REQUIRE((pipeline && output));
    device->writeRWStructuredBuffer(4, {.buffer = output, .count = 16, .stride = 4});
    slag::CommandBuffer* commandBuffer =
        device->createCommandBuffer({.queueType = slag::CommandQueueType::Compute}).value();
    REQUIRE(static_cast<bool>(commandBuffer->begin()));
    commandBuffer->bindPipeline(pipeline);
    const uint32_t push[] = {100, 101, 102, 4}; // slots 100-102 were never written
    commandBuffer->pushConstants(push, sizeof(push));
    commandBuffer->dispatch(1, 1, 1);
    REQUIRE(static_cast<bool>(commandBuffer->end()));
    slag::Semaphore* semaphore = device->createSemaphore({}).value();
    slag::SemaphoreValue done = {semaphore, 1};
    slag::CommandBuffer* commandBuffers[] = {commandBuffer};
    REQUIRE(static_cast<bool>(device->commandQueue(slag::CommandQueueType::Compute)
                                  ->submit({.commandBuffers = commandBuffers, .signals = {&done, 1}})));
    (void)device->waitSemaphores({&done, 1}, slag::WaitMode::All, 10'000'000'000);
    size_t uninitialized = 0;
    for (const std::string& error : messages.errors)
    {
        uninitialized += error.find("Uninitialized descriptor") != std::string::npos ? 1 : 0;
    }
    MESSAGE("GPU-based validation: " << uninitialized << " uninitialized-descriptor reports of "
                                     << messages.errors.size() << " errors");
    CHECK(uninitialized == 3);
    CHECK(messages.errors.size() == 3);
    device->destroy(semaphore);
    device->destroy(commandBuffer);
    device->destroy(output);
    device->destroy(pipeline);
    device->shutdown();
    slag::destroy(instance);
}
#endif
