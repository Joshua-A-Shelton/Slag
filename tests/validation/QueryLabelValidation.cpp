// Validation of query pools, labels, and defragmentation passes.

#include "ValidationFixture.h"

#include <algorithm>
#include <vector>

using namespace slag;

TEST_CASE("query pools")
{
    ValidationFixture f;
    CHECK_VALIDATION(f, "count is 0", (void)f.device->createQueryPool({.count = 0}));
    CHECK_VALIDATION(f, "occlusion pools are for graphics queues",
                     (void)f.device->createQueryPool(
                         {.type = QueryType::Occlusion, .count = 1, .queueType = CommandQueueType::Compute}));
    f.common->capabilities.copyQueueTimestamps = false;
    CHECK_VALIDATION(f, "copy-queue timestamps are not supported",
                     (void)f.device->createQueryPool({.count = 1, .queueType = CommandQueueType::Copy}));
    f.common->capabilities.copyQueueTimestamps = true;
    f.common->capabilities.timestampCalibration = false;
    CHECK_VALIDATION(f, "timestamp calibration is not supported",
                     (void)f.device->commandQueue(CommandQueueType::Graphics)->calibrateTimestamps());

    QueryPool* timestamps = f.device->createQueryPool({.count = 2}).value();
    QueryPool* copyTimestamps = f.device->createQueryPool({.count = 2, .queueType = CommandQueueType::Copy}).value();
    QueryPool* occlusion = f.device->createQueryPool({.type = QueryType::Occlusion, .count = 2}).value();
    Buffer* results = f.buffer(64);
    CommandBuffer* idle = f.device->createCommandBuffer({}).value();
    CHECK_VALIDATION(f, "not recording", idle->writeTimestamp(timestamps, 0));
    CHECK_VALIDATION(f, "not recording, or inside a render pass", idle->resolveQueries(timestamps, 0, 1, results, 0));
    CommandBuffer* commandBuffer = f.recording();
    CHECK_VALIDATION(f, "index 2 is out of range", commandBuffer->writeTimestamp(timestamps, 2));
    CHECK_VALIDATION(f, "the pool doesn't hold timestamps", commandBuffer->writeTimestamp(occlusion, 0));
    CHECK_VALIDATION(f, "copy-queue pools are only used on copy queues",
                     commandBuffer->writeTimestamp(copyTimestamps, 0));
    CHECK_VALIDATION(f, "the range is outside it", commandBuffer->resolveQueries(timestamps, 1, 2, results, 0));
    CHECK_VALIDATION(f, "copy-queue pools are only used on copy queues",
                     commandBuffer->resolveQueries(copyTimestamps, 0, 1, results, 0));
    CHECK_VALIDATION(f, "the offset isn't a multiple of 8",
                     commandBuffer->resolveQueries(timestamps, 0, 1, results, 4));

    // Occlusion queries.
    CHECK_VALIDATION(f, "not inside a render pass", commandBuffer->beginQuery(occlusion, 0));
    commandBuffer->endQuery(occlusion, 0); // closes the one the failed call opened
    Texture* target = f.texture(TextureUsage::RenderTarget);
    AttachmentView* view = f.device->createAttachmentView({.texture = target}).value();
    const ColorAttachment color = {.view = view};
    commandBuffer->beginRendering({.colorAttachments = {&color, 1}});
    CHECK_VALIDATION(f, "not an occlusion pool", commandBuffer->beginQuery(timestamps, 0));
    commandBuffer->endQuery(timestamps, 0);
    commandBuffer->beginQuery(occlusion, 0);
    CHECK_VALIDATION(f, "another occlusion query is still open", commandBuffer->beginQuery(occlusion, 1));
    CHECK_VALIDATION(f, "query 0 of this pool isn't the open one", commandBuffer->endQuery(occlusion, 0));
    commandBuffer->beginQuery(occlusion, 0);
    CHECK_VALIDATION(f, "an occlusion query is still open", commandBuffer->endRendering());
    commandBuffer->endQuery(occlusion, 0);

    ValidationFixture other;
    QueryPool* foreign = other.device->createQueryPool({.count = 1}).value();
    CHECK_VALIDATION(f, "the query pool belongs to another device", f.device->destroy(foreign));
    other.device->destroy(foreign);

    f.device->destroy(results);
    CHECK_VALIDATION(f, "use of a destroyed Buffer", commandBuffer->resolveQueries(timestamps, 0, 1, results, 0));
    f.device->destroy(timestamps);
    CHECK_VALIDATION(f, "use of a destroyed QueryPool", commandBuffer->writeTimestamp(timestamps, 0));
    Buffer* alive = f.buffer(64);
    CHECK_VALIDATION(f, "use of a destroyed QueryPool", commandBuffer->resolveQueries(timestamps, 0, 1, alive, 0));
    commandBuffer->beginRendering({.colorAttachments = {&color, 1}});
    CHECK_VALIDATIONS(f, 2, "use of a destroyed QueryPool", commandBuffer->beginQuery(timestamps, 0)); // not occlusion
    CHECK_VALIDATION(f, "use of a destroyed QueryPool", commandBuffer->endQuery(timestamps, 0));
    commandBuffer->endRendering();
    CHECK_VALIDATION(f, "use of a destroyed QueryPool", (void)timestamps->userData());
    CHECK_VALIDATION(f, "use of a destroyed QueryPool", timestamps->setUserData(1));
    CHECK_VALIDATION(f, "use of a destroyed QueryPool", f.device->destroy(timestamps));

    f.device->destroy(idle);
    CHECK_VALIDATIONS(f, 3, "use of a destroyed CommandBuffer", idle->writeTimestamp(occlusion, 0)); // not timestamps
    CHECK_VALIDATIONS(f, 2, "use of a destroyed CommandBuffer", idle->resolveQueries(occlusion, 0, 1, alive, 0));
    CHECK_VALIDATIONS(f, 2, "use of a destroyed CommandBuffer", idle->beginQuery(occlusion, 0));
    CHECK_VALIDATIONS(f, 2, "use of a destroyed CommandBuffer", idle->endQuery(occlusion, 1));
    f.device->destroy(view);
    f.device->destroy(target);
}

TEST_CASE("labels")
{
    ValidationFixture f;
    CommandBuffer* idle = f.device->createCommandBuffer({}).value();
    CHECK_VALIDATION(f, "not recording", idle->beginLabel("label"));
    CHECK_VALIDATION(f, "not recording", idle->endLabel());
    CHECK_VALIDATION(f, "not recording", idle->insertLabel("label"));
    CommandBuffer* commandBuffer = f.recording();
    CHECK_VALIDATION(f, "name is null", commandBuffer->beginLabel(nullptr));
    commandBuffer->endLabel();
    CHECK_VALIDATION(f, "name is null", commandBuffer->insertLabel(nullptr));
    CHECK_VALIDATION(f, "no label is open", commandBuffer->endLabel());
    f.device->destroy(idle);
    CHECK_VALIDATIONS(f, 2, "use of a destroyed CommandBuffer", idle->beginLabel("label"));
    CHECK_VALIDATIONS(f, 2, "use of a destroyed CommandBuffer", idle->endLabel());
    CHECK_VALIDATIONS(f, 2, "use of a destroyed CommandBuffer", idle->insertLabel("label"));
}

TEST_CASE("defragmentation passes")
{
    ValidationFixture f;
    // Fragmented GpuOnly memory: three quarters of the buffers freed.
    std::vector<Buffer*> buffers;
    std::vector<Texture*> textures;
    for (uint32_t i = 0; i < 1024; ++i)
    {
        buffers.push_back(
            f.device
                ->createBuffer(
                    {.size = 1 << 20, .placement = Placement::automatic(MemoryKind::GpuOnly, {.movable = true})})
                .value());
        if (i % 8 == 0) // so every block holds textures too
        {
            textures.push_back(
                f.device
                    ->createTexture2D({.format = PixelFormat::R8G8B8A8Unorm,
                                       .width = 64,
                                       .height = 64,
                                       .placement = Placement::automatic(MemoryKind::GpuOnly, {.movable = true})})
                    .value());
        }
    }
    for (size_t i = 0; i < buffers.size(); ++i)
    {
        if (i % 4 != 0)
        {
            f.device->destroy(buffers[i]);
            buffers[i] = nullptr;
        }
    }
    CHECK_VALIDATION(f, "only GpuOnly and GpuUpload memory is defragmented",
                     (void)f.device->beginDefragmentation({.kind = MemoryKind::Upload}));
    // Small passes, so every pass below still has moves.
    DefragmentationPass* pass = f.device->beginDefragmentation({.maxMoves = 100}).value();
    REQUIRE(pass);
    REQUIRE_FALSE(pass->moves().empty());
    CHECK_VALIDATION(f, "a defragmentation pass is already active", (void)f.device->beginDefragmentation({}));
    Buffer* movedBuffer = nullptr;
    Texture* movedTexture = nullptr;
    for (const DefragmentationMove& move : pass->moves())
    {
        movedBuffer = move.buffer && !movedBuffer ? move.buffer : movedBuffer;
        movedTexture = move.texture && !movedTexture ? move.texture : movedTexture;
    }
    REQUIRE((movedBuffer && movedTexture));
    f.device->writeRawBuffer(7, {.buffer = movedBuffer, .size = 256}); // left stale on purpose
    CHECK_VALIDATION(f, "is being moved by the active defragmentation pass", f.device->destroy(movedTexture));
    textures.erase(std::find(textures.begin(), textures.end(), movedTexture));

    CHECK_VALIDATIONS(f, 2, "the pass has moves, but recordCopies wasn't called",
                      (void)f.device->endDefragmentation(pass, {})); // and no completion value
    pass = f.device->beginDefragmentation({.maxMoves = 4}).value();
    REQUIRE(pass);
    REQUIRE_FALSE(pass->moves().empty());
    CHECK_VALIDATION(f, "not this device's active pass", (void)f.device->endDefragmentation(nullptr, {}));
    CommandBuffer* idle = f.device->createCommandBuffer({}).value();
    CHECK_VALIDATION(f, "the command buffer is not recording", pass->recordCopies(idle));
    CommandBuffer* commandBuffer = f.recording();
    Texture* target = f.texture(TextureUsage::RenderTarget);
    AttachmentView* view = f.device->createAttachmentView({.texture = target}).value();
    const ColorAttachment color = {.view = view};
    commandBuffer->beginRendering({.colorAttachments = {&color, 1}});
    // The failed call above still recorded, so this reports both.
    CHECK_VALIDATIONS(f, 2, "inside a render pass", pass->recordCopies(commandBuffer));
    commandBuffer->endRendering();
    CHECK_VALIDATION(f, "the copies are already recorded", pass->recordCopies(commandBuffer));
    f.device->destroy(idle);
    CHECK_VALIDATIONS(f, 3, "use of a destroyed CommandBuffer", pass->recordCopies(idle));
    Semaphore* semaphore = f.device->createSemaphore({}).value();
    CHECK_VALIDATION(f, "hasn't been reached", (void)f.device->endDefragmentation(pass, {semaphore, 1}));

    // The slot written with a moved buffer is reported at the next submit, once.
    (void)commandBuffer->end();
    CommandBuffer* list[] = {commandBuffer};
    CommandQueue* queue = f.device->commandQueue(CommandQueueType::Graphics);
    CHECK_VALIDATION(f, "resource slot 7 still holds the descriptor of a resource moved",
                     (void)queue->submit({.commandBuffers = list}));
    CHECK_NO_VALIDATION(f, (void)queue->submit({}));

    pass = f.device->beginDefragmentation({.maxMoves = 4}).value();
    REQUIRE(pass);
    for (const DefragmentationMove& move : pass->moves())
    {
        if (move.buffer)
        {
            CHECK_VALIDATION(f, "is being moved by the active defragmentation pass", f.device->destroy(move.buffer));
            buffers.erase(std::find(buffers.begin(), buffers.end(), move.buffer));
            break;
        }
    }
    (void)f.device->endDefragmentation(pass, {});
    f.messages.errors.clear();
    for (Buffer* buffer : buffers)
    {
        f.device->destroy(buffer);
    }
    for (Texture* texture : textures)
    {
        f.device->destroy(texture);
    }
    f.device->destroy(view);
    f.device->destroy(target);
    f.device->destroy(semaphore);
}
