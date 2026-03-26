#include <gtest/gtest.h>
#include <slag/Slag.h>
#include "../utilities/GeneralUtilities.h"
using namespace slag;

TEST(CommandBuffer, Create)
{
    auto card = Slag::backend()->graphicsCard(0);
    auto queueType = sequentialEnumRange(QueueType::GRAPHICS,QueueType::TRANSFER);
    for (int i=0; i < queueType.size(); i++)
    {
        auto cb = std::unique_ptr<CommandBuffer>(card->newCommandBuffer(queueType[i]));
        GTEST_ASSERT_EQ(cb->graphicsCard(),card);
        GTEST_ASSERT_EQ(cb->type(),queueType[i]);
    }
}
//This test is really a "Trust me bro". There's no real way to check the barriers work, only that they don't work, so this really just checks for thrown errors
TEST(CommandBuffer, InsertBarriers)
{
    auto card = Slag::backend()->graphicsCard(0);
    auto commandBuffer = std::unique_ptr<CommandBuffer>(card->newCommandBuffer(QueueType::GRAPHICS));
    auto finished = std::unique_ptr<Semaphore>(card->newSemaphore());
    auto sourceBuffer = std::unique_ptr<Buffer>(card->newBuffer(256,BufferCPUAccess::WRITE_ONLY));
    auto destinationBuffer = std::unique_ptr<Buffer>(card->newBuffer(256,BufferCPUAccess::READ_WRITE));
    auto destinationTexture = std::unique_ptr<Texture>(card->newTexture2D(32,32,PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::SAMPLED));
    auto srcBufferPtr = sourceBuffer->as<uint8_t>();
    for (auto i=0; i< sourceBuffer->size(); i++)
    {
        srcBufferPtr[i] = i;
    }
    commandBuffer->begin();
    commandBuffer->copyBufferToBuffer(sourceBuffer.get(),0,destinationBuffer.get(),0,128);
    BufferBarrier bufferBarrier
    {
        .buffer = destinationBuffer.get(),
        .offset = 0,
        .length = 256,
        .syncBefore = SyncStages::COPY,
        .syncAfter = SyncStages::COPY,
        .flush = MemoryCaches::COPY_WRITE,
        .invalidate = MemoryCaches::NONE
    };
    commandBuffer->insertBarriers(&bufferBarrier,1);
    TextureBufferMapping mapping
    {
        .bufferOffset = 0,
        .subresource =
        {
            .aspect = PixelAspect::COLOR,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .offset = {0,0,0},
        .extent = {32,1,1}
    };
    commandBuffer->copyBufferToTexture(sourceBuffer.get(),destinationTexture.get(),&mapping,1);
    TextureBarrier textureBarrier
    {
        .texture = destinationTexture.get(),
        .syncBefore = SyncStages::COPY,
        .syncAfter = SyncStages::COPY,
        .flush = MemoryCaches::COPY_WRITE,
        .invalidate = MemoryCaches::NONE,
    };
    commandBuffer->insertBarriers(&textureBarrier,1);
    commandBuffer->copyBufferToBuffer(sourceBuffer.get(),0,destinationBuffer.get(),128,128);
    mapping.offset.y = 1;
    commandBuffer->copyBufferToTexture(sourceBuffer.get(),destinationTexture.get(),&mapping,1);
    GlobalBarrier globalBarrier
    {
        .syncBefore = SyncStages::COPY,
        .syncAfter = SyncStages::COPY,
        .flush = MemoryCaches::COPY_WRITE,
        .invalidate = MemoryCaches::COPY_READ,
    };
    commandBuffer->insertBarriers(&globalBarrier,1);
    mapping.offset.y = 2;
    commandBuffer->copyBufferToTexture(sourceBuffer.get(),destinationTexture.get(),&mapping,1);
    commandBuffer->insertBarriers(nullptr,0,nullptr,0,&textureBarrier,1);
    commandBuffer->end();

    SemaphoreValue signal{.semaphore = finished.get(), .value = 1};
    auto cb = commandBuffer.get();
    SubmissionBatch submissionBatch{};
    submissionBatch.commandBuffers = &cb;
    submissionBatch.commandBufferCount = 1;
    submissionBatch.signalSemaphores = &signal;
    submissionBatch.signalSemaphoreCount = 1;
    card->graphicsQueue()->submit(&submissionBatch,1);
    finished->waitForValue(1);

}

TEST(CommandBuffer, CopyBufferToBuffer)
{
    auto card = Slag::backend()->graphicsCard(0);
    auto commandBuffer = std::unique_ptr<CommandBuffer>(card->newCommandBuffer(QueueType::TRANSFER));
    auto srcBuffer = std::unique_ptr<Buffer>(card->newBuffer(128,BufferCPUAccess::WRITE_ONLY,BufferMemoryType::GENERAL));
    auto dstBuffer = std::unique_ptr<Buffer>(card->newBuffer(128,BufferCPUAccess::READ_WRITE,BufferMemoryType::GENERAL));
    auto finished = std::unique_ptr<Semaphore>(card->newSemaphore());

    auto srcPtr = srcBuffer->as<uint8_t>();
    auto dstPtr = dstBuffer->as<uint8_t>();

    for (uint8_t i = 0; i < 128; i++)
    {
        srcPtr[i] = i;
    }

    commandBuffer->begin();
    commandBuffer->copyBufferToBuffer(srcBuffer.get(),0,dstBuffer.get(),64,64);
    commandBuffer->copyBufferToBuffer(srcBuffer.get(),64,dstBuffer.get(),0,64);
    commandBuffer->end();

    auto cmdBufferPtr = commandBuffer.get();
    SemaphoreValue signal
    {
        .semaphore = finished.get(),
        .value = 1
    };
    SubmissionBatch batch
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = &cmdBufferPtr,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1,
    };

    card->transferQueue()->submit(&batch,1);
    finished->waitForValue(1);
    for (uint8_t i=0; i < 128; i++)
    {
        if (i<64)
        {
            GTEST_ASSERT_EQ(dstPtr[i+64],i);
        }
        else
        {
            GTEST_ASSERT_EQ(dstPtr[i-64],i);
        }
    }
}

TEST(CommandBuffer, CopyBufferToTextureToBuffer)
{
    auto card = Slag::backend()->graphicsCard(0);
    auto commandBuffer = std::unique_ptr<CommandBuffer>(card->newCommandBuffer(QueueType::TRANSFER));
    auto texture = std::unique_ptr<Texture>(card->newTexture2D(64,64,PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::SAMPLED,3));
    auto finished = std::unique_ptr<Semaphore>(card->newSemaphore());
    auto srcBuffer = std::unique_ptr<Buffer>(card->newBuffer((32*32*4)+32,BufferCPUAccess::WRITE_ONLY));
    auto dstBuffer = std::unique_ptr<Buffer>(card->newBuffer(srcBuffer->size(),BufferCPUAccess::READ_WRITE));
    auto srcBufferPtr = srcBuffer->as<uint8_t>();
    for (auto i=16; i<srcBuffer->size()-16; i++)
    {
        srcBufferPtr[i] = 172;
    }
    auto dstPtr = dstBuffer->as<uint8_t>();
    for (auto i=0; i<dstBuffer->size(); i++)
    {
        dstPtr[i] = 0;
    }

    commandBuffer->begin();
    TextureBufferMapping mapping
    {
        .bufferOffset = 16,
        .subresource =
        {
            .aspect = PixelAspect::COLOR,
            .mipLevel = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .offset = {0,0,0},
        .extent = {32,32,1}
    };
    commandBuffer->copyBufferToTexture(srcBuffer.get(),texture.get(),&mapping,1);
    TextureBarrier textureBarrier
    {
        .texture = texture.get(),
        .syncBefore = SyncStages::COPY,
        .syncAfter = SyncStages::COPY,
        .flush = MemoryCaches::COPY_WRITE,
        .invalidate = MemoryCaches::COPY_READ,
    };
    commandBuffer->insertBarriers(&textureBarrier,1);
    commandBuffer->copyTextureToBuffer(texture.get(),dstBuffer.get(),&mapping,1);
    commandBuffer->end();

    auto cb = commandBuffer.get();
    SemaphoreValue signal{.semaphore = finished.get(),.value = 1};
    SubmissionBatch batch{};
    batch.commandBuffers = &cb;
    batch.commandBufferCount = 1;
    batch.signalSemaphores = &signal;
    batch.signalSemaphoreCount = 1;
    card->transferQueue()->submit(&batch,1);
    finished->waitForValue(1);

    for (int i=0; i< dstBuffer->size(); i++)
    {
        if (i<16 || i>= dstBuffer->size()-16)
        {
            GTEST_ASSERT_EQ(dstPtr[i],0);
        }
        else
        {
            GTEST_ASSERT_EQ(dstPtr[i],172);
        }
    }
}


#ifdef SLAG_DEBUG
TEST(CommandBuffer, TransferErrorComputeCommands)
{
    GTEST_FAIL();
}
TEST(CommandBuffer, TransferErrorGraphicsCommands)
{
    GTEST_FAIL();
}
TEST(CommandBuffer, ComputeErrorGraphicsCommands)
{
    GTEST_FAIL();
}
#endif
