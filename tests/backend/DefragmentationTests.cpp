#include "TestDevice.h"

#include <slag/AttachmentView.h>
#include <slag/Buffer.h>
#include <slag/CommandBuffer.h>
#include <slag/Defragmentation.h>
#include <slag/GraphicsDevice.h>
#include <slag/Semaphore.h>
#include <slag/Texture.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace {

constexpr uint32_t bufferCount = 1024;
constexpr uint64_t bufferSize = 1 << 20;
constexpr uint32_t stampSize = 256; // written at both ends of each buffer
constexpr uint32_t textureEvery = 32;
constexpr uint32_t textureSize = 128;
constexpr uint32_t textureMips = 3;

struct TestTexture
{
    slag::Texture* texture = nullptr;
    slag::AttachmentView* view = nullptr; // every other texture is a render target
    uint32_t id = 0;
};

uint8_t stampByte(uint32_t id, uint32_t i)
{
    return static_cast<uint8_t>(id * 31 + i * 7 + 1);
}

// Runs command buffers in order across queues: each submit waits for the last.
struct Submitter
{
    slag::GraphicsDevice* device = nullptr;
    slag::Semaphore* semaphore = nullptr;
    uint64_t value = 0;

    slag::CommandBuffer* begin(slag::CommandQueueType type)
    {
        slag::CommandBuffer* commandBuffer = device->createCommandBuffer({.queueType = type}).value();
        REQUIRE(commandBuffer);
        REQUIRE(static_cast<bool>(commandBuffer->begin()));
        return commandBuffer;
    }

    // Returns the value that says it's done.
    slag::SemaphoreValue submit(slag::CommandBuffer* commandBuffer, slag::CommandQueueType type)
    {
        REQUIRE(static_cast<bool>(commandBuffer->end()));
        slag::SemaphoreValue wait = {semaphore, value};
        slag::SemaphoreValue signal = {semaphore, ++value};
        slag::CommandBuffer* list[] = {commandBuffer};
        REQUIRE(static_cast<bool>(device->commandQueue(type)->submit(
            {.waits = {&wait, 1}, .commandBuffers = list, .signals = {&signal, 1}})));
        REQUIRE(static_cast<bool>(device->waitSemaphores({&signal, 1}, slag::WaitMode::All, 10'000'000'000)));
        device->destroy(commandBuffer);
        return signal;
    }
};

uint64_t usage(slag::GraphicsDevice* device)
{
    slag::MemoryBudget budget = device->memoryBudget();
    return budget.dedicated.usageBytes + budget.shared.usageBytes;
}

// Fragments GpuOnly memory with movable buffers and textures, keeps a
// quarter of the buffers and half the textures, and runs passes until
// nothing moves (ADR 0062): memory is given back, everything reads back
// unchanged, and a moved texture's attachment view still renders.
void testDefragmentation(slag::Backend backend)
{
    TestInstance test(backend);
    slag::GraphicsDevice* device = test.firstDevice();
    REQUIRE(device);
    MESSAGE("device: " << std::string(device->info().name));
    REQUIRE(static_cast<bool>(device->initialize({})));
    Submitter submitter{device, device->createSemaphore({}).value()};
    REQUIRE(submitter.semaphore);

    slag::CopyFootprint footprints[textureMips];
    std::vector<slag::Buffer*> buffers;
    std::vector<TestTexture> textures;
    for (uint32_t i = 0; i < bufferCount; ++i)
    {
        slag::Buffer* buffer =
            device
                ->createBuffer({.size = bufferSize,
                                .placement = slag::Placement::automatic(slag::MemoryKind::GpuOnly, {.movable = true}),
                                .userData = i})
                .value();
        REQUIRE(buffer);
        buffers.push_back(buffer);
        if (i % textureEvery == 0)
        {
            TestTexture entry;
            entry.id = i / textureEvery;
            bool target = entry.id % 4 == 0;
            entry.texture =
                device
                    ->createTexture2D(
                        {.format = slag::PixelFormat::R8G8B8A8Unorm,
                         .width = textureSize,
                         .height = textureSize,
                         .mipCount = textureMips,
                         .usage = target ? slag::TextureUsage::RenderTarget : slag::TextureUsage::None,
                         .placement = slag::Placement::automatic(slag::MemoryKind::GpuOnly, {.movable = true})})
                    .value();
            REQUIRE(entry.texture);
            if (target)
            {
                entry.view = device->createAttachmentView({.texture = entry.texture}).value();
                REQUIRE(entry.view);
            }
            textures.push_back(entry);
        }
    }
    for (uint32_t mip = 0; mip < textureMips; ++mip)
    {
        footprints[mip] = device->copyFootprint(textures[0].texture, mip);
    }
    uint64_t textureBytes = 0;
    for (const slag::CopyFootprint& footprint : footprints)
    {
        textureBytes += footprint.size;
    }

    // Stamps: both ends of every buffer, every mip of every texture.
    uint64_t textureStaging = uint64_t(bufferCount) * stampSize;
    slag::Buffer* staging = device
                                ->createBuffer({.size = textureStaging + textureBytes * textures.size(),
                                                .placement = slag::Placement::automatic(slag::MemoryKind::Upload)})
                                .value();
    REQUIRE(staging);
    auto* bytes = static_cast<uint8_t*>(staging->mappedData());
    for (uint32_t id = 0; id < bufferCount; ++id)
    {
        for (uint32_t i = 0; i < stampSize; ++i)
        {
            bytes[uint64_t(id) * stampSize + i] = stampByte(id, i);
        }
    }
    for (size_t t = 0; t < textures.size(); ++t)
    {
        uint8_t* texels = bytes + textureStaging + t * textureBytes;
        for (uint64_t i = 0; i < textureBytes; ++i)
        {
            texels[i] = stampByte(10'000 + textures[t].id, static_cast<uint32_t>(i));
        }
    }
    slag::CommandBuffer* upload = submitter.begin(slag::CommandQueueType::Graphics);
    std::vector<slag::TextureBarrier> toCopy;
    for (const TestTexture& entry : textures)
    {
        toCopy.push_back({.syncAfter = slag::BarrierSync::Copy,
                          .accessAfter = slag::BarrierAccess::CopyDest,
                          .layoutBefore = slag::TextureLayout::Undefined,
                          .layoutAfter = slag::TextureLayout::CopyDest,
                          .texture = entry.texture});
    }
    upload->barrier({}, {}, toCopy);
    for (uint32_t id = 0; id < bufferCount; ++id)
    {
        upload->copyBuffer(buffers[id], 0, staging, uint64_t(id) * stampSize, stampSize);
        upload->copyBuffer(buffers[id], bufferSize - stampSize, staging, uint64_t(id) * stampSize, stampSize);
    }
    for (size_t t = 0; t < textures.size(); ++t)
    {
        uint64_t offset = textureStaging + t * textureBytes;
        for (uint32_t mip = 0; mip < textureMips; ++mip)
        {
            upload->copyBufferToTexture({.texture = textures[t].texture, .mip = mip},
                                        {staging, offset, footprints[mip].rowPitch}, textureSize >> mip,
                                        textureSize >> mip);
            offset += footprints[mip].size;
        }
    }
    // Rest in Common, so passes can run on any queue.
    for (slag::TextureBarrier& barrier : toCopy)
    {
        barrier = {.syncBefore = slag::BarrierSync::Copy,
                   .accessBefore = slag::BarrierAccess::CopyDest,
                   .layoutBefore = slag::TextureLayout::CopyDest,
                   .layoutAfter = slag::TextureLayout::Common,
                   .texture = barrier.texture};
    }
    upload->barrier({}, {}, toCopy);
    submitter.submit(upload, slag::CommandQueueType::Graphics);

    // Keep a quarter of the buffers and half the textures.
    std::vector<slag::Buffer*> keptBuffers;
    for (uint32_t id = 0; id < bufferCount; ++id)
    {
        if (id % 4 == 0)
        {
            keptBuffers.push_back(buffers[id]);
        }
        else
        {
            device->destroy(buffers[id]);
        }
    }
    std::vector<TestTexture> keptTextures;
    for (TestTexture& entry : textures)
    {
        if (entry.id % 2 == 0)
        {
            keptTextures.push_back(entry);
        }
        else
        {
            device->destroy(entry.view);
            device->destroy(entry.texture);
        }
    }
    uint64_t before = usage(device);

    // Passes until nothing moves: the first limited to 64 moves, then
    // alternating copy and graphics queues.
    uint32_t passes = 0;
    size_t movedBuffers = 0;
    size_t movedTextures = 0;
    for (;; ++passes)
    {
        REQUIRE(passes < 64);
        slag::Result<slag::DefragmentationPass*> begun =
            device->beginDefragmentation({.maxMoves = passes == 0 ? 64u : 0u});
        REQUIRE(static_cast<bool>(begun));
        slag::DefragmentationPass* pass = begun.value();
        std::span<const slag::DefragmentationMove> moves = pass->moves();
        if (moves.empty())
        {
            REQUIRE(static_cast<bool>(device->endDefragmentation(pass, {})));
            break;
        }
        CHECK(moves.size() <= (passes == 0 ? 64u : moves.size()));
        slag::CommandQueueType queue =
            passes % 2 == 0 ? slag::CommandQueueType::Graphics : slag::CommandQueueType::Copy;
        slag::CommandBuffer* commandBuffer = submitter.begin(queue);
        std::vector<slag::TextureBarrier> toSource;
        for (const slag::DefragmentationMove& move : moves)
        {
            movedBuffers += move.buffer ? 1 : 0;
            movedTextures += move.texture ? 1 : 0;
            if (move.texture && queue != slag::CommandQueueType::Copy)
            {
                toSource.push_back({.syncAfter = slag::BarrierSync::Copy,
                                    .accessAfter = slag::BarrierAccess::CopySource,
                                    .layoutBefore = slag::TextureLayout::Common,
                                    .layoutAfter = slag::TextureLayout::CopySource,
                                    .texture = move.texture});
            }
        }
        commandBuffer->barrier({}, {}, toSource);
        pass->recordCopies(commandBuffer);
        // Back to Common: the moved textures come out in CopySource.
        for (slag::TextureBarrier& barrier : toSource)
        {
            barrier = {.layoutBefore = slag::TextureLayout::CopySource,
                       .layoutAfter = slag::TextureLayout::Common,
                       .texture = barrier.texture};
        }
        commandBuffer->barrier({}, {}, toSource);
        slag::SemaphoreValue done = submitter.submit(commandBuffer, queue);
        REQUIRE(static_cast<bool>(device->endDefragmentation(pass, done)));
    }
    uint64_t after = usage(device);
    MESSAGE(passes << " passes moved " << movedBuffers << " buffers and " << movedTextures << " textures; usage "
                   << before / (1 << 20) << " MB -> " << after / (1 << 20) << " MB");
    CHECK(passes >= 2);
    CHECK(movedBuffers > 0);
    CHECK(movedTextures > 0);
    // Three quarters of the buffer memory was freed; at least a third of it
    // should go back as whole blocks.
    CHECK(before - after >= uint64_t(bufferCount) * bufferSize / 4);

    // A moved render target's view still works: clear mip 0 through it.
    TestTexture* target = nullptr;
    for (TestTexture& entry : keptTextures)
    {
        target = entry.view ? &entry : target;
    }
    REQUIRE(target);
    slag::CommandBuffer* clear = submitter.begin(slag::CommandQueueType::Graphics);
    slag::TextureBarrier toTarget = {.syncAfter = slag::BarrierSync::RenderTarget,
                                     .accessAfter = slag::BarrierAccess::RenderTarget,
                                     .layoutBefore = slag::TextureLayout::Common,
                                     .layoutAfter = slag::TextureLayout::RenderTarget,
                                     .texture = target->texture,
                                     .range = {.baseMip = 0, .mipCount = 1}};
    clear->barrier({}, {}, {&toTarget, 1});
    const slag::ColorAttachment color = {
        .view = target->view, .loadOp = slag::LoadOp::Clear, .clearValue = {.color = {1.0f, 0.0f, 1.0f, 1.0f}}};
    clear->beginRendering({.colorAttachments = {&color, 1}});
    clear->endRendering();
    toTarget = {.syncBefore = slag::BarrierSync::RenderTarget,
                .accessBefore = slag::BarrierAccess::RenderTarget,
                .layoutBefore = slag::TextureLayout::RenderTarget,
                .layoutAfter = slag::TextureLayout::Common,
                .texture = target->texture,
                .range = {.baseMip = 0, .mipCount = 1}};
    clear->barrier({}, {}, {&toTarget, 1});
    submitter.submit(clear, slag::CommandQueueType::Graphics);

    // Read every stamp back.
    uint64_t readbackTextures = uint64_t(keptBuffers.size()) * 2 * stampSize;
    slag::Buffer* readback = device
                                 ->createBuffer({.size = readbackTextures + textureBytes * keptTextures.size(),
                                                 .placement = slag::Placement::automatic(slag::MemoryKind::Readback)})
                                 .value();
    REQUIRE(readback);
    slag::CommandBuffer* read = submitter.begin(slag::CommandQueueType::Graphics);
    std::vector<slag::TextureBarrier> toRead;
    for (const TestTexture& entry : keptTextures)
    {
        toRead.push_back({.syncAfter = slag::BarrierSync::Copy,
                          .accessAfter = slag::BarrierAccess::CopySource,
                          .layoutBefore = slag::TextureLayout::Common,
                          .layoutAfter = slag::TextureLayout::CopySource,
                          .texture = entry.texture});
    }
    read->barrier({}, {}, toRead);
    for (size_t k = 0; k < keptBuffers.size(); ++k)
    {
        read->copyBuffer(readback, k * 2 * stampSize, keptBuffers[k], 0, stampSize);
        read->copyBuffer(readback, k * 2 * stampSize + stampSize, keptBuffers[k], bufferSize - stampSize, stampSize);
    }
    for (size_t k = 0; k < keptTextures.size(); ++k)
    {
        uint64_t offset = readbackTextures + k * textureBytes;
        for (uint32_t mip = 0; mip < textureMips; ++mip)
        {
            read->copyTextureToBuffer({readback, offset, footprints[mip].rowPitch},
                                      {.texture = keptTextures[k].texture, .mip = mip}, textureSize >> mip,
                                      textureSize >> mip);
            offset += footprints[mip].size;
        }
    }
    submitter.submit(read, slag::CommandQueueType::Graphics);

    const auto* result = static_cast<const uint8_t*>(readback->mappedData());
    uint32_t badBuffers = 0;
    for (size_t k = 0; k < keptBuffers.size(); ++k)
    {
        auto id = static_cast<uint32_t>(keptBuffers[k]->userData());
        bool good = true;
        for (uint32_t i = 0; i < 2 * stampSize; ++i)
        {
            good = good && result[k * 2 * stampSize + i] == stampByte(id, i % stampSize);
        }
        badBuffers += good ? 0 : 1;
    }
    CHECK(badBuffers == 0);
    uint32_t badTextures = 0;
    for (size_t k = 0; k < keptTextures.size(); ++k)
    {
        const uint8_t* texels = result + readbackTextures + k * textureBytes;
        bool cleared = &keptTextures[k] == target;
        bool good = true;
        uint64_t offset = 0;
        for (uint32_t mip = 0; mip < textureMips; ++mip)
        {
            uint32_t size = textureSize >> mip;
            for (uint32_t row = 0; row < size; ++row)
            {
                for (uint32_t x = 0; x < size * 4; ++x)
                {
                    uint64_t at = offset + uint64_t(row) * footprints[mip].rowPitch + x;
                    uint8_t expected = stampByte(10'000 + keptTextures[k].id, static_cast<uint32_t>(at));
                    if (cleared && mip == 0)
                    {
                        const uint8_t magenta[4] = {255, 0, 255, 255};
                        expected = magenta[x % 4];
                    }
                    good = good && texels[at] == expected;
                }
            }
            offset += footprints[mip].size;
        }
        badTextures += good ? 0 : 1;
    }
    CHECK(badTextures == 0);

    device->destroy(readback);
    device->destroy(staging);
    for (slag::Buffer* buffer : keptBuffers)
    {
        device->destroy(buffer);
    }
    for (TestTexture& entry : keptTextures)
    {
        device->destroy(entry.view);
        device->destroy(entry.texture);
    }
    device->destroy(submitter.semaphore);
    device->shutdown();
    test.checkNoErrors();
}

#ifdef SLAG_VALIDATION
// A slot still holding a moved buffer's descriptor is reported at the next
// submit, once; a slot written again is not (ADR 0062).
void testStaleSlots(slag::Backend backend)
{
    TestInstance test(backend);
    slag::GraphicsDevice* device = test.firstDevice();
    REQUIRE(device);
    REQUIRE(static_cast<bool>(device->initialize({})));
    Submitter submitter{device, device->createSemaphore({}).value()};

    // Two blocks' worth of buffers, then all but two freed, so a pass moves them.
    std::vector<slag::Buffer*> buffers;
    for (uint32_t i = 0; i < bufferCount; ++i)
    {
        buffers.push_back(
            device
                ->createBuffer({.size = bufferSize,
                                .placement = slag::Placement::automatic(slag::MemoryKind::GpuOnly, {.movable = true})})
                .value());
        REQUIRE(buffers.back());
    }
    for (uint32_t i = 0; i < bufferCount; ++i)
    {
        if (i % 4 != 0)
        {
            device->destroy(buffers[i]);
            buffers[i] = nullptr;
        }
    }
    slag::DefragmentationPass* pass = device->beginDefragmentation({}).value();
    REQUIRE(pass);
    REQUIRE(pass->moves().size() >= 2);
    slag::Buffer* first = pass->moves()[0].buffer;
    slag::Buffer* second = pass->moves()[1].buffer;
    REQUIRE((first && second));
    device->writeRawBuffer(3, {.buffer = first, .offset = 0, .size = bufferSize});
    device->writeRawBuffer(4, {.buffer = second, .offset = 0, .size = bufferSize});
    slag::CommandBuffer* commandBuffer = submitter.begin(slag::CommandQueueType::Graphics);
    pass->recordCopies(commandBuffer);
    slag::SemaphoreValue done = submitter.submit(commandBuffer, slag::CommandQueueType::Graphics);
    REQUIRE(static_cast<bool>(device->endDefragmentation(pass, done)));
    test.checkNoErrors();

    device->writeRawBuffer(4, {.buffer = second, .offset = 0, .size = bufferSize}); // fixed
    submitter.submit(submitter.begin(slag::CommandQueueType::Graphics), slag::CommandQueueType::Graphics);
    REQUIRE(test.messages.errors.size() == 1);
    CHECK(test.messages.errors[0].find("slot 3") != std::string::npos);
    submitter.submit(submitter.begin(slag::CommandQueueType::Graphics), slag::CommandQueueType::Graphics);
    CHECK(test.messages.errors.size() == 1); // reported once
    test.messages.errors.clear();

    for (slag::Buffer* buffer : buffers)
    {
        device->destroy(buffer);
    }
    device->destroy(submitter.semaphore);
    device->shutdown();
    test.checkNoErrors();
}

// Destroying a resource in an active pass drops its move; ending with a
// semaphore value not yet reached is reported (ADR 0062).
void testPassValidation(slag::Backend backend)
{
    TestInstance test(backend);
    slag::GraphicsDevice* device = test.firstDevice();
    REQUIRE(device);
    REQUIRE(static_cast<bool>(device->initialize({})));
    slag::Semaphore* semaphore = device->createSemaphore({}).value();
    std::vector<slag::Buffer*> buffers;
    for (uint32_t i = 0; i < bufferCount; ++i)
    {
        slag::Buffer* buffer =
            device
                ->createBuffer({.size = bufferSize,
                                .placement = slag::Placement::automatic(slag::MemoryKind::GpuOnly, {.movable = true})})
                .value();
        REQUIRE(buffer);
        buffers.push_back(buffer);
    }
    // Freed after all are created, so the space isn't reused.
    for (uint32_t i = 0; i < bufferCount; ++i)
    {
        if (i % 4 != 0)
        {
            device->destroy(buffers[i]);
            buffers[i] = nullptr;
        }
    }
    slag::DefragmentationPass* pass = device->beginDefragmentation({}).value();
    REQUIRE(pass);
    REQUIRE_FALSE(pass->moves().empty());
    slag::Buffer* moved = pass->moves()[0].buffer;
    REQUIRE(moved);
    device->destroy(moved);
    REQUIRE(test.messages.errors.size() == 1);
    CHECK(test.messages.errors[0].find("being moved") != std::string::npos);
    CHECK(pass->moves()[0].buffer == nullptr);

    slag::CommandBuffer* commandBuffer = device->createCommandBuffer({}).value();
    REQUIRE(static_cast<bool>(commandBuffer->begin()));
    pass->recordCopies(commandBuffer);
    REQUIRE(static_cast<bool>(commandBuffer->end()));
    (void)device->endDefragmentation(pass, {semaphore, 1}); // never submitted
    REQUIRE(test.messages.errors.size() == 2);
    CHECK(test.messages.errors[1].find("hasn't been reached") != std::string::npos);
    test.messages.errors.clear();

    device->destroy(commandBuffer);
    for (slag::Buffer* buffer : buffers)
    {
        if (buffer != moved)
        {
            device->destroy(buffer);
        }
    }
    device->destroy(semaphore);
    device->shutdown();
    test.checkNoErrors();
}
#endif

} // namespace

#ifdef SLAG_BACKEND_VULKAN
TEST_CASE("Vulkan: defragmentation frees blocks and moved resources keep their contents")
{
    testDefragmentation(slag::Backend::Vulkan);
}
#ifdef SLAG_VALIDATION
TEST_CASE("Vulkan: validation reports descriptor slots left stale by defragmentation")
{
    testStaleSlots(slag::Backend::Vulkan);
}
TEST_CASE("Vulkan: validation reports destroying moved resources and unreached pass values")
{
    testPassValidation(slag::Backend::Vulkan);
}
#endif
#endif

#ifdef SLAG_BACKEND_D3D12
TEST_CASE("D3D12: defragmentation frees blocks and moved resources keep their contents")
{
    testDefragmentation(slag::Backend::D3D12);
}
#ifdef SLAG_VALIDATION
TEST_CASE("D3D12: validation reports descriptor slots left stale by defragmentation")
{
    testStaleSlots(slag::Backend::D3D12);
}
TEST_CASE("D3D12: validation reports destroying moved resources and unreached pass values")
{
    testPassValidation(slag::Backend::D3D12);
}
#endif
#endif
