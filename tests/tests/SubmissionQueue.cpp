#include <gtest/gtest.h>
#include <slag/Slag.h>
using namespace slag;
#ifdef SLAG_DEBUG
TEST(SubmissionQueue, ErrorOnZeroSubmit)
{
    auto card = Slag::backend()->graphicsCard(0);
    SubmissionBatch batch{};
    ASSERT_DEATH(card->transferQueue()->submit(&batch,0),"At least one batch must be submitted");
}
TEST(SubmissionQueue, ErrorOnNullptrSubmit)
{
    auto card = Slag::backend()->graphicsCard(0);
    ASSERT_DEATH(card->transferQueue()->submit(nullptr,1),"Parameter \"batches\" must not be nullptr");
}
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
    EXPECT_DEATH(card->transferQueue()->submit(&graphicsBatch,1),"Queue cannot process command buffer outside it's capabilities");
    EXPECT_DEATH(card->transferQueue()->submit(&computeBatch,1),"Queue cannot process command buffer outside it's capabilities");
}
TEST(SubmissionQueue, ErrorOnSubmitFrameTransfer)
{
    GTEST_FAIL();
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

    EXPECT_DEATH(card->computeQueue()->submit(&graphicsBatch,1),"Queue cannot process command buffer outside it's capabilities");
}
TEST(SubmissionQueue, ErrorOnSubmitFrameCompute)
{
    GTEST_FAIL();
}
#endif

TEST(SubmissionQueue,Submit)
{
    GTEST_FAIL();
}

TEST(SubmissionQueue, SubmitFrame)
{
    GTEST_FAIL();
}