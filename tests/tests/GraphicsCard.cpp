#include <gtest/gtest.h>
#include <slag/Slag.h>
using namespace slag;
TEST(GraphicsCard, Name)
{
    GTEST_ASSERT_TRUE(Slag::backend()->graphicsCardCount() > 0);
    for (auto i=0u; i< Slag::backend()->graphicsCardCount(); i++)
    {
        GraphicsCard* card = Slag::backend()->graphicsCard(i);
        GTEST_ASSERT_TRUE(card->name().length()>1);
    }
}
TEST(GraphicsCard, VideoMemory)
{
    GTEST_ASSERT_TRUE(Slag::backend()->graphicsCardCount() > 0);
    for (auto i=0u; i< Slag::backend()->graphicsCardCount(); i++)
    {
        GraphicsCard* card = Slag::backend()->graphicsCard(i);
        GTEST_ASSERT_TRUE(card->videoMemory()>0);
    }
}

TEST(GraphicsCard, MaxShaderAccessReadOnlyBufferSize)
{
    GTEST_ASSERT_TRUE(Slag::backend()->graphicsCardCount() > 0);
    for (auto i=0u; i< Slag::backend()->graphicsCardCount(); i++)
    {
        GraphicsCard* card = Slag::backend()->graphicsCard(i);
        GTEST_ASSERT_TRUE(card->maxShaderAccessReadOnlyBufferSize()>=16384);
    }
}

TEST(GraphicsCard, DefragmentAll)
{
    auto card = Slag::backend()->graphicsCard(0);
    auto buffer1 = std::unique_ptr<Buffer>(card->newBuffer(256));
    auto buffer2 = std::unique_ptr<Buffer>(card->newBuffer(64));
    auto buffer3 = std::unique_ptr<Buffer>(card->newBuffer(256));
    auto buffer4 = std::unique_ptr<Buffer>(card->newBuffer(128));
    auto buffer6 = std::unique_ptr<Buffer>(card->newBuffer(512));
    auto buffer7 = std::unique_ptr<Buffer>(card->newBuffer(512));
    auto buffer8 = std::unique_ptr<Buffer>(card->newBuffer(256));
    //auto texture = std::unique_ptr<Texture>(card->newTexture());
    auto uploadBuffer = std::unique_ptr<Buffer>(card->newBuffer(256,BufferUsage::ARBITRARY,BufferShaderAccess::READ_ONLY,BufferCPUAccess::READ_WRITE));
    auto data = uploadBuffer->as<uint8_t>();
    for (int i=0; i< 256; i++)
    {
        data[i] = i;
    }
    auto finished = std::unique_ptr<Semaphore>(card->newSemaphore());

    auto virtualAddress = buffer8->deviceAddress();

    auto commandBuffer = std::unique_ptr<CommandBuffer>(card->newCommandBuffer(QueueType::TRANSFER));
    commandBuffer->begin();
    commandBuffer->copyBufferToBuffer(uploadBuffer.get(),0,buffer8.get(),0,256);
    commandBuffer->end();

    SemaphoreValue signal
    {
        .semaphore = finished.get(),
        .value = 1
    };
    auto cbuffer = commandBuffer.get();
    SubmissionBatch batch
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = &cbuffer,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1,
    };

    card->transferQueue()->submit(&batch,1);
    finished->waitForValue(1);
    buffer2.release();
    buffer6.release();
    GTEST_ASSERT_EQ(virtualAddress, buffer8->deviceAddress());
    auto defragmentFinished = std::unique_ptr<Semaphore>(card->newSemaphore());
    signal.semaphore = defragmentFinished.get();
    card->defragmentMemory(nullptr,0,&signal,1);
    GTEST_ASSERT_NE(virtualAddress, buffer8->deviceAddress());

    auto downloadData = std::unique_ptr<Buffer>(card->newBuffer(256,BufferUsage::ARBITRARY,BufferShaderAccess::READ_WRITE,BufferCPUAccess::READ_WRITE));
    auto transferFinished = std::unique_ptr<Semaphore>(card->newSemaphore());
    commandBuffer->begin();
    commandBuffer->copyBufferToBuffer(buffer8.get(),0,downloadData.get(),0,256);
    commandBuffer->end();

    signal.semaphore = transferFinished.get();
    cbuffer = commandBuffer.get();
    card->transferQueue()->submit(&batch,1);
    transferFinished->waitForValue(1);
    data = downloadData->as<uint8_t>();
    for (int i=0; i< 256; i++)
    {
        GTEST_ASSERT_EQ(data[i], i);
    }
}

TEST(GraphicsCard, DefragmentTarget)
{
    GTEST_FAIL();
}