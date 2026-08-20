#include <chrono>
#include <gtest/gtest.h>
#include <slag/Slag.h>
#include "../utilities/PlatformUtilities.h"
#include "slag/exceptions/ResourceCreationError.h"
using namespace slag;

TEST(SwapChain, Timing)
{
    const uint32_t FRAME_COUNTER = 1000;
    auto graphicsCard = Slag::backend()->graphicsCard(0);
    auto swapchain = utilities::createSwapChain(graphicsCard,500,500,PixelFormat::B8G8R8A8_UNORM,PresentMode::IMMEDIATE,2);
    auto commandBuffer = std::unique_ptr<CommandBuffer>(graphicsCard->newCommandBuffer(QueueType::GRAPHICS));
    auto finished = std::unique_ptr<Semaphore>(graphicsCard->newSemaphore(0));

    auto dummyBuffer1 = std::unique_ptr<Buffer>(graphicsCard->newBuffer(256));
    auto dummyBuffer2 = std::unique_ptr<Buffer>(graphicsCard->newBuffer(256));
    uint32_t frameCount = 0;
    auto tearingStart = std::chrono::high_resolution_clock::now();
    while (frameCount < FRAME_COUNTER)
    {
        auto frame = swapchain->next();
        commandBuffer->begin();
        TextureBarrier tb
        {
            .texture = frame->renderBuffer(),
            .baseMipLevel = 0,
            .mipCount = 1,
            .baseLayer = 0,
            .layerCount = 1,
            .syncBefore = SyncStages::NONE,
            .syncAfter = SyncStages::ALL,
            .flush = MemoryCaches::NONE,
            .invalidate = MemoryCaches::NONE,
            .layoutBefore = TextureLayout::UNKNOWN,
            .layoutAfter = TextureLayout::PRESENT
        };
        commandBuffer->insertBarriers(&tb,1);
        commandBuffer->copyBufferToBuffer(dummyBuffer1.get(),0,dummyBuffer2.get(),0,dummyBuffer1->size());
        commandBuffer->end();
        auto cb = commandBuffer.get();
        SemaphoreValue signal{.semaphore = finished.get(), .value = frameCount+1};
        graphicsCard->graphicsQueue()->submit({.waitSemaphores = nullptr, .waitSemaphoreCount = 0, .commandBuffers = &cb, .commandBufferCount = 1, .signalSemaphores = &signal, .signalSemaphoreCount = 1});
        swapchain->present();
        finished->waitForValue(frameCount+1);
        frameCount++;
    }

    auto tearingEnd = std::chrono::high_resolution_clock::now();
    auto tearingTime = std::chrono::duration_cast<std::chrono::microseconds>(tearingEnd - tearingStart);
    finished = std::unique_ptr<Semaphore>(graphicsCard->newSemaphore(0));
    frameCount=0;
    swapchain->setParameters(SwapChainParameters{.presentMode = PresentMode::QUEUE, .imageCount = 2, .imageFormat = PixelFormat::B8G8R8A8_UNORM});

    auto doubleBufferStart = std::chrono::high_resolution_clock::now();
    while (frameCount < FRAME_COUNTER)
    {
        auto frame = swapchain->next();
        commandBuffer->begin();
        TextureBarrier tb
        {
            .texture = frame->renderBuffer(),
            .baseMipLevel = 0,
            .mipCount = 1,
            .baseLayer = 0,
            .layerCount = 1,
            .syncBefore = SyncStages::NONE,
            .syncAfter = SyncStages::ALL,
            .flush = MemoryCaches::NONE,
            .invalidate = MemoryCaches::NONE,
            .layoutBefore = TextureLayout::UNKNOWN,
            .layoutAfter = TextureLayout::PRESENT
        };
        commandBuffer->insertBarriers(&tb,1);
        commandBuffer->copyBufferToBuffer(dummyBuffer1.get(),0,dummyBuffer2.get(),0,dummyBuffer1->size());
        commandBuffer->end();
        auto cb = commandBuffer.get();
        SemaphoreValue signal{.semaphore = finished.get(), .value = frameCount+1};
        graphicsCard->graphicsQueue()->submit({.waitSemaphores = nullptr, .waitSemaphoreCount = 0, .commandBuffers = &cb, .commandBufferCount = 1, .signalSemaphores = &signal, .signalSemaphoreCount = 1});
        swapchain->present();
        finished->waitForValue(frameCount+1);
        frameCount++;
    }
    auto doubleBufferEnd = std::chrono::high_resolution_clock::now();
    auto doubleBufferTime = std::chrono::duration_cast<std::chrono::microseconds>(doubleBufferEnd - doubleBufferStart);

    finished = std::unique_ptr<Semaphore>(graphicsCard->newSemaphore(0));
    frameCount=0;
    swapchain->setParameters(SwapChainParameters{.presentMode = PresentMode::BUFFER, .imageCount = 3, .imageFormat = PixelFormat::B8G8R8A8_UNORM});

    auto tripleBufferStart = std::chrono::high_resolution_clock::now();
    while (frameCount < FRAME_COUNTER)
    {
        auto frame = swapchain->next();
        commandBuffer->begin();
        TextureBarrier tb
        {
            .texture = frame->renderBuffer(),
            .baseMipLevel = 0,
            .mipCount = 1,
            .baseLayer = 0,
            .layerCount = 1,
            .syncBefore = SyncStages::NONE,
            .syncAfter = SyncStages::ALL,
            .flush = MemoryCaches::NONE,
            .invalidate = MemoryCaches::NONE,
            .layoutBefore = TextureLayout::UNKNOWN,
            .layoutAfter = TextureLayout::PRESENT
        };
        commandBuffer->insertBarriers(&tb,1);
        commandBuffer->copyBufferToBuffer(dummyBuffer1.get(),0,dummyBuffer2.get(),0,dummyBuffer1->size());
        commandBuffer->end();
        auto cb = commandBuffer.get();
        SemaphoreValue signal{.semaphore = finished.get(), .value = frameCount+1};
        graphicsCard->graphicsQueue()->submit({.waitSemaphores = nullptr, .waitSemaphoreCount = 0, .commandBuffers = &cb, .commandBufferCount = 1, .signalSemaphores = &signal, .signalSemaphoreCount = 1});
        swapchain->present();
        finished->waitForValue(frameCount+1);
        frameCount++;
    }
    auto tripleBufferEnd = std::chrono::high_resolution_clock::now();
    auto tripleBufferTime = std::chrono::duration_cast<std::chrono::microseconds>(tripleBufferEnd - tripleBufferStart);


    std::cout << "Tearing:       Total - " << tearingTime.count() << " µs, Average - " << ((double)tearingTime.count())/FRAME_COUNTER << " µs, FPS - " <<  1000000/((double)tearingTime.count()/FRAME_COUNTER)<< std::endl;
    std::cout << "Double Buffer: Total - " << doubleBufferTime.count() << " µs, Average - " << ((double)doubleBufferTime.count())/FRAME_COUNTER << " µs, FPS - " <<  1000000/((double)doubleBufferTime.count()/FRAME_COUNTER)<< std::endl;
    std::cout << "Triple Buffer: Total - " << tripleBufferTime.count() << " µs, Average - " << ((double)tripleBufferTime.count())/FRAME_COUNTER << " µs, FPS - " <<  1000000/((double)tripleBufferTime.count()/FRAME_COUNTER)<< std::endl;
    GTEST_ASSERT_GT(doubleBufferTime,tearingTime);
    GTEST_ASSERT_GT(doubleBufferTime,tripleBufferTime);
}

TEST(SwapChain, ErrorIfLessThan2Images)
{
    auto graphicsCard = Slag::backend()->graphicsCard(0);
    EXPECT_NO_THROW(utilities::createSwapChain(graphicsCard,500,500,PixelFormat::B8G8R8A8_UNORM_SRGB,PresentMode::IMMEDIATE,2));
    EXPECT_THROW(utilities::createSwapChain(graphicsCard,500,500,PixelFormat::B8G8R8A8_UNORM_SRGB,PresentMode::IMMEDIATE,1),ResourceCreationError);
}