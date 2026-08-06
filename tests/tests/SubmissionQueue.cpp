#include <gtest/gtest.h>
#include <slag/Slag.h>
using namespace slag;
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