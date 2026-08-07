#include <gtest/gtest.h>
#include <slag/Slag.h>
using namespace slag;
TEST(SubmissionQueue, MultiSubmit)
{
    auto card = Slag::backend()->graphicsCard(0);
    auto commandBuffer = std::unique_ptr<CommandBuffer>(card->newCommandBuffer(QueueType::TRANSFER));
    auto commandBuffer2 = std::unique_ptr<CommandBuffer>(card->newCommandBuffer(QueueType::TRANSFER));
    auto commandBuffer3 = std::unique_ptr<CommandBuffer>(card->newCommandBuffer(QueueType::TRANSFER));
    auto firstFinished = std::unique_ptr<Semaphore>(card->newSemaphore());
    auto secondFinished = std::unique_ptr<Semaphore>(card->newSemaphore());
    auto thirdFinished = std::unique_ptr<Semaphore>(card->newSemaphore());
    auto sourceBuffer = std::unique_ptr<Buffer>(card->newBuffer(256,BufferCPUAccess::READ_WRITE));
    auto destinationBuffer = std::unique_ptr<Buffer>(card->newBuffer(256,BufferCPUAccess::READ_WRITE));
    auto srcPtr = sourceBuffer->as<uint32_t>();
    auto dstPtr = destinationBuffer->as<uint32_t>();
    for (auto i = 0u; i < 64; i++)
    {
        srcPtr[i] = i;
        dstPtr[i] = 0;
    }
    commandBuffer->begin();
    commandBuffer->copyBufferToBuffer(sourceBuffer.get(),128,destinationBuffer.get(),0,128);
    BufferBarrier barrier
    {
        .buffer = destinationBuffer.get(),
        .offset = 0,
        .length = 256,
        .syncBefore = SyncStages::ALL,
        .syncAfter = SyncStages::ALL,
        .flush = MemoryCaches::COPY_WRITE,
        .invalidate = MemoryCaches::NONE,
    };
    commandBuffer->insertBarriers(&barrier,1);
    commandBuffer->end();

    commandBuffer2->begin();
    commandBuffer2->copyBufferToBuffer(sourceBuffer.get(),0,destinationBuffer.get(),128,128);
    commandBuffer2->insertBarriers(&barrier,1);
    commandBuffer2->end();

    commandBuffer3->begin();
    commandBuffer3->copyBufferToBuffer(sourceBuffer.get(),0,destinationBuffer.get(),0,256);
    commandBuffer3->insertBarriers(&barrier,1);
    commandBuffer3->end();


    auto cmdBuffer = commandBuffer.get();
    auto cmdBuffer2 = commandBuffer2.get();
    auto cmdBuffer3 = commandBuffer3.get();

    SemaphoreValue firstFinishedSV{.semaphore = firstFinished.get(),.value =1};
    SemaphoreValue secondFinishedSV[]
    {
        SemaphoreValue {.semaphore = secondFinished.get(),.value =1},
        SemaphoreValue {.semaphore = firstFinished.get(),.value =2}
    };
    SemaphoreValue thirdBeginSV{.semaphore = firstFinished.get(), .value = 3};
    SemaphoreValue thirdFinishedSV{.semaphore = thirdFinished.get(), .value = 1};

    SubmissionBatch batches[]
    {
          SubmissionBatch
          {
              .waitSemaphores = nullptr,
              .waitSemaphoreCount = 0,
              .commandBuffers = &cmdBuffer,
              .commandBufferCount = 1,
              .signalSemaphores = &firstFinishedSV,
              .signalSemaphoreCount = 1,
          },
        SubmissionBatch
        {
            .waitSemaphores = &firstFinishedSV,
            .waitSemaphoreCount = 1,
            .commandBuffers = &cmdBuffer2,
            .commandBufferCount = 1,
            .signalSemaphores = secondFinishedSV,
            .signalSemaphoreCount = 2,
        },
        SubmissionBatch
        {
            .waitSemaphores = &thirdBeginSV,
            .waitSemaphoreCount = 1,
            .commandBuffers = &cmdBuffer3,
            .commandBufferCount = 1,
            .signalSemaphores = &thirdFinishedSV,
            .signalSemaphoreCount = 1,
        }
    };
    card->transferQueue()->submit(batches,3);
    secondFinished->waitForValue(1);
    firstFinished->waitForValue(2);
    GTEST_ASSERT_EQ(secondFinished->value(),1);
    GTEST_ASSERT_EQ(firstFinished->value(),2);
    for (auto i = 0u; i < 32; i++)
    {
        GTEST_ASSERT_EQ(dstPtr[i+32],i);
    }
    for (auto i = 0u; i < 32; i++)
    {
        GTEST_ASSERT_EQ(dstPtr[i],i+32);
    }
    firstFinished->signal(3);
    thirdFinished->waitForValue(1);
    for (auto i = 0u; i < 64; i++)
    {
        GTEST_ASSERT_EQ(dstPtr[i],i);
    }
}
#ifdef SLAG_DEBUG
TEST(SubmissionQueue, ErrorOnSubmitTransfer)
{
    auto card = Slag::backend()->graphicsCard(0);
    auto graphicsBuffer = std::unique_ptr<CommandBuffer>(card->newCommandBuffer(QueueType::GRAPHICS));
    auto computeBuffer = std::unique_ptr<CommandBuffer>(card->newCommandBuffer(QueueType::COMPUTE));
    auto dataBuffer = std::unique_ptr<Buffer>(card->newBuffer(256));
    graphicsBuffer->begin();
    graphicsBuffer->copyBufferToBuffer(dataBuffer.get(),0,dataBuffer.get(),128,128);
    graphicsBuffer->end();

    computeBuffer->begin();
    computeBuffer->copyBufferToBuffer(dataBuffer.get(),0,dataBuffer.get(),128,128);
    computeBuffer->end();

    auto gb = graphicsBuffer.get();
    auto cb = computeBuffer.get();

    SubmissionBatch graphicsBatch{};
    graphicsBatch.commandBuffers = &gb;
    graphicsBatch.commandBufferCount = 1;

    SubmissionBatch computeBatch{};
    computeBatch.commandBuffers = &cb;
    computeBatch.commandBufferCount = 1;
    EXPECT_DEATH(card->transferQueue()->submit(graphicsBatch),"Queue cannot process command buffer outside it's capabilities");
    EXPECT_DEATH(card->transferQueue()->submit(computeBatch),"Queue cannot process command buffer outside it's capabilities");
}
TEST(SubmissionQueue, ErrorOnSubmitFrameTransfer)
{
    auto card = Slag::backend()->graphicsCard(0);
    auto graphicsBuffer = std::unique_ptr<CommandBuffer>(card->newCommandBuffer(QueueType::GRAPHICS));

    auto dataBuffer = std::unique_ptr<Buffer>(card->newBuffer(256));
    graphicsBuffer->begin();
    graphicsBuffer->copyBufferToBuffer(dataBuffer.get(),0,dataBuffer.get(),128,128);
    graphicsBuffer->end();

    auto gb = graphicsBuffer.get();

    SubmissionBatch graphicsBatch{};
    graphicsBatch.commandBuffers = &gb;
    graphicsBatch.commandBufferCount = 1;

    EXPECT_DEATH(card->transferQueue()->submit(graphicsBatch),"Queue cannot process command buffer outside it's capabilities");
    EXPECT_DEATH(card->computeQueue()->submit(graphicsBatch),"Queue cannot process command buffer outside it's capabilities");

    auto computeBuffer = std::unique_ptr<CommandBuffer>(card->newCommandBuffer(QueueType::COMPUTE));
    graphicsBuffer->begin();
    graphicsBuffer->copyBufferToBuffer(dataBuffer.get(),0,dataBuffer.get(),128,128);
    graphicsBuffer->end();

    auto cb = computeBuffer.get();
    SubmissionBatch computeBatch{};
    computeBatch.commandBuffers = &cb;
    computeBatch.commandBufferCount = 1;
    EXPECT_DEATH(card->transferQueue()->submit(computeBatch),"Queue cannot process command buffer outside it's capabilities");

}
TEST(SubmissionQueue, ErrorOnSubmitCompute)
{
    auto card = Slag::backend()->graphicsCard(0);
    auto graphicsBuffer = std::unique_ptr<CommandBuffer>(card->newCommandBuffer(QueueType::GRAPHICS));

    auto dataBuffer = std::unique_ptr<Buffer>(card->newBuffer(256));
    graphicsBuffer->begin();
    graphicsBuffer->copyBufferToBuffer(dataBuffer.get(),0,dataBuffer.get(),128,128);
    graphicsBuffer->end();

    auto gb = graphicsBuffer.get();

    SubmissionBatch graphicsBatch{};
    graphicsBatch.commandBuffers = &gb;
    graphicsBatch.commandBufferCount = 1;

    EXPECT_DEATH(card->computeQueue()->submit(graphicsBatch),"Queue cannot process command buffer outside it's capabilities");
}
#endif