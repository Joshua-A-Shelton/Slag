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
        GTEST_ASSERT_EQ(cb->level(),CommandBufferLevel::PRIMARY);
    }
}

TEST(CommandBuffer, SubBuffer)
{
    GTEST_FAIL();
}

TEST(CommandBuffer, CopyBufferToBuffer)
{
    auto card = Slag::backend()->graphicsCard(0);
    auto commandBuffer = std::unique_ptr<CommandBuffer>(card->newCommandBuffer(QueueType::TRANSFER));
    auto srcBuffer = std::unique_ptr<Buffer>(card->newBuffer(128,BufferMemoryType::GENERAL,BufferCPUAccess::WRITE_ONLY));
    auto dstBuffer = std::unique_ptr<Buffer>(card->newBuffer(128,BufferMemoryType::GENERAL,BufferCPUAccess::READ_WRITE));
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

TEST(CommandBuffer, CopyBufferToTexture)
{
    GTEST_FAIL();
}

TEST(CommandBuffer, CopyTextureToBuffer)
{
    GTEST_FAIL();
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
