#include <gtest/gtest.h>
#include <slag/Slag.h>
using namespace slag;

TEST(Semaphore, Create)
{
    auto card = Slag::backend()->graphicsCard(0);
    auto semaphore1 = std::unique_ptr<Semaphore>(card->newSemaphore());
    auto semaphore2 = std::unique_ptr<Semaphore>(card->newSemaphore(3));
    GTEST_ASSERT_EQ(semaphore1->value(), 0);
    GTEST_ASSERT_EQ(semaphore2->value(), 3);
}

TEST(Semaphore, Signal)
{
    auto card = Slag::backend()->graphicsCard(0);
    auto semaphore = std::unique_ptr<Semaphore>(card->newSemaphore(0));
    semaphore->signal(1);
    GTEST_ASSERT_EQ(semaphore->value(), 1);
    semaphore->signal(5);
    GTEST_ASSERT_EQ(semaphore->value(), 5);
}

TEST(Semaphore, WaitForValue)
{
    auto card = Slag::backend()->graphicsCard(0);
    auto semaphore = std::unique_ptr<Semaphore>(card->newSemaphore(0));
    int tracker = 0;

    std::thread waitThread([&]()
    {
        semaphore->waitForValue(1);
        tracker = 1;
    });
    tracker = 2;
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    GTEST_ASSERT_EQ(tracker, 2);
    semaphore->signal(1);
    waitThread.join();
    GTEST_ASSERT_EQ(tracker, 1);
}

TEST(Semaphore, WaitAndSignalFromGPU)
{
    auto card = Slag::backend()->graphicsCard(0);
    auto commandBuffer = std::unique_ptr<CommandBuffer>(card->newCommandBuffer(QueueType::TRANSFER));
    auto semaphore1 = std::unique_ptr<Semaphore>(card->newSemaphore(0));
    auto semaphore2 = std::unique_ptr<Semaphore>(card->newSemaphore(0));
    auto sourceBuffer = std::unique_ptr<Buffer>(card->newBuffer(512,BufferCPUAccess::WRITE_ONLY,BufferMemoryType::UNIFORM));
    auto destinationBuffer = std::unique_ptr<Buffer>(card->newBuffer(512));

    //fill the source with some data, doesn't really matter what
    auto dataPtr = sourceBuffer->as<uint8_t>();
    for (int i=0; i< 512; i++)
    {
        dataPtr[i] = 7;
    }

    commandBuffer->begin();
    commandBuffer->copyBufferToBuffer(sourceBuffer.get(),0,destinationBuffer.get(),0,512);
    commandBuffer->end();
    SemaphoreValue signal
    {
        .semaphore = semaphore1.get(),
        .value = 1
    };
    auto bufferPtr = commandBuffer.get();
    SubmissionBatch batch
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = &bufferPtr,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1,
    };
    auto start = std::chrono::steady_clock::now();
    card->transferQueue()->submit(batch);
    semaphore1->waitForValue(1);
    auto end = std::chrono::steady_clock::now();

    GTEST_ASSERT_EQ(semaphore1->value(), 1);
    auto firstTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    //set it to new value
    for (int i=0; i< 512; i++)
    {
        dataPtr[i] = 6;
    }

    commandBuffer->begin();
    commandBuffer->copyBufferToBuffer(sourceBuffer.get(),0,destinationBuffer.get(),0,512);
    commandBuffer->end();

    signal.semaphore = semaphore2.get();
    SemaphoreValue wait
    {
        .semaphore = semaphore1.get(),
        .value = 2
    };

    batch.waitSemaphores = &wait;
    batch.waitSemaphoreCount = 1;

    start = std::chrono::steady_clock::now();
    card->transferQueue()->submit(batch);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    semaphore1->signal(2);
    semaphore2->waitForValue(1);
    end = std::chrono::steady_clock::now();

    auto secondTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    GTEST_ASSERT_EQ(semaphore1->value(), 2);
    GTEST_ASSERT_EQ(semaphore2->value(), 1);
    GTEST_ASSERT_GT(secondTime, firstTime);
    auto totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    GTEST_ASSERT_TRUE(totalTime <= std::chrono::milliseconds(220) && totalTime >= std::chrono::milliseconds(180) );
}