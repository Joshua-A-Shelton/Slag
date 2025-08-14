#include <gtest/gtest.h>
#include <slag/Slag.h>
#include "../Utilities.h"

using namespace slag;

class TestingResources: public FrameResources
{
public:

    CommandBuffer* commandBuffer = nullptr;
    TestingResources()
    {
        commandBuffer = CommandBuffer::newCommandBuffer(GPUQueue::QueueType::GRAPHICS);
    }
    ~TestingResources()override
    {
        delete commandBuffer;
    }

};

FrameResources* createResources(uint8_t frameIndex, SwapChain* inSwapchain)
{
    return new TestingResources();
}


int64_t renderEmptyFrames(SwapChain* swapchain, uint64_t frameCount, ClearColor clearColor)
{
    auto submitQueue = slagGraphicsCard()->graphicsQueue();

    auto startTime = SDL_GetTicks64();
    for (int i=0; i< frameCount; i++)
    {
        if (auto frame = swapchain->next())
        {
            auto resources = frame->frameResources<TestingResources>();
            auto renderTexture = frame->backBuffer();
            auto commandBuffer = resources->commandBuffer;
            commandBuffer->begin();
            commandBuffer->clearTexture(renderTexture,clearColor);
            commandBuffer->end();

            submitQueue->submit(frame,&commandBuffer,1,nullptr,0,nullptr,0);
        }
    }
    auto endTime = SDL_GetTicks64();
    return endTime - startTime;
}

uint64_t renderAttemptsEmptyFrames(SwapChain* swapchain, uint64_t successfulFrames, ClearColor clearColor)
{
    auto submitQueue = slagGraphicsCard()->graphicsQueue();
    uint64_t loops = 0;
    auto successes = 0;
    while (successes < successfulFrames)
    {
        if (auto frame = swapchain->nextIfReady())
        {
            auto resources = frame->frameResources<TestingResources>();
            auto renderTexture = frame->backBuffer();
            auto commandBuffer = resources->commandBuffer;
            commandBuffer->begin();
            commandBuffer->clearTexture(renderTexture,clearColor);
            commandBuffer->end();

            submitQueue->submit(frame,&commandBuffer,1,nullptr,0,nullptr,0);
            successes++;
        }
        loops++;
        if (loops == UINT64_MAX)
        {
            break;
        }
    }
    return loops;

}
TEST(SwapChain, PresentModes)
{
    auto window = utilities::createWindow("Present Modes",150,150);
    auto swapchain = utilities::createSwapChain(window.get(),1,SwapChain::PresentMode::IMMEDIATE,Pixels::Format::B8G8R8A8_UNORM_SRGB,createResources);

    auto tearTime = renderEmptyFrames(swapchain.get(),300,ClearColor{1,0,0,1});
    swapchain->presentMode(SwapChain::PresentMode::QUEUE,2);
    auto doubleBufferTime = renderEmptyFrames(swapchain.get(),300,ClearColor{0,1,0,1});
    swapchain->presentMode(SwapChain::PresentMode::BUFFER,3);
    auto tripleBufferTime = renderEmptyFrames(swapchain.get(),300,ClearColor{0,0,1,1});
    swapchain->presentMode(SwapChain::PresentMode::QUEUE,3);
    auto queueTime = renderEmptyFrames(swapchain.get(),300,ClearColor{1,1,0,1});

    GTEST_ASSERT_LT(tearTime,doubleBufferTime);
    GTEST_ASSERT_LT(tripleBufferTime, doubleBufferTime);
    GTEST_ASSERT_TRUE(queueTime > tripleBufferTime);
    GTEST_ASSERT_TRUE(tearTime < 300);
    GTEST_ASSERT_TRUE(tripleBufferTime < 300);
    auto closeness = std::abs(1.0-((double)tearTime/(double)tripleBufferTime));
    GTEST_ASSERT_TRUE(closeness < .5);
}

TEST(SwapChain, NextIfReady)
{
    auto window = utilities::createWindow("Next If Ready",150,150);
    auto swapchain = utilities::createSwapChain(window.get(),1,SwapChain::PresentMode::IMMEDIATE,Pixels::Format::B8G8R8A8_UNORM_SRGB,createResources);

    auto immediateAttempts = renderAttemptsEmptyFrames(swapchain.get(),300,ClearColor{1,0,0,1});
    GTEST_ASSERT_TRUE(immediateAttempts != UINT64_MAX);
    swapchain->presentMode(SwapChain::PresentMode::QUEUE,2);
    auto doubleBufferAttempts = renderEmptyFrames(swapchain.get(),300,ClearColor{0,1,0,1});
    GTEST_ASSERT_TRUE(doubleBufferAttempts != UINT64_MAX);
    swapchain->presentMode(SwapChain::PresentMode::BUFFER,3);
    auto tripleBufferAttempts = renderEmptyFrames(swapchain.get(),300,ClearColor{0,0,1,1});
    GTEST_ASSERT_TRUE(tripleBufferAttempts != UINT64_MAX);

    GTEST_ASSERT_EQ(immediateAttempts, 300);
    GTEST_ASSERT_TRUE(doubleBufferAttempts > 300);
    GTEST_ASSERT_EQ(tripleBufferAttempts, 300);
}

TEST(SwapChain, Resize)
{
    uint32_t windowSize = 150;
    auto window = utilities::createWindow("Resize",windowSize,windowSize);
    auto swapchain = utilities::createSwapChain(window.get(),2,SwapChain::PresentMode::BUFFER,Pixels::Format::B8G8R8A8_UNORM_SRGB,createResources);
    auto submitQueue = slagGraphicsCard()->graphicsQueue();

    SDL_Event event;
    for (auto i = 0; i<99; i++)
    {
        while (SDL_PollEvent(&event) != 0)
        {
            if (event.type == SDL_WINDOWEVENT_RESIZED)
            {
                int w,h;
                SDL_GetWindowSize(window.get(),&w,&h);
                swapchain->backBufferSize(w,h);
            }
        }
        if (auto frame = swapchain->next())
        {

            auto resources = frame->frameResources<TestingResources>();
            auto renderTexture = frame->backBuffer();
            GTEST_ASSERT_TRUE(renderTexture->width() == windowSize);
            GTEST_ASSERT_TRUE(renderTexture->height() == windowSize);
            auto commandBuffer = resources->commandBuffer;
            commandBuffer->begin();
            commandBuffer->clearTexture(renderTexture,ClearColor{1,0,1,1});
            commandBuffer->end();

            submitQueue->submit(frame,&commandBuffer,1,nullptr,0,nullptr,0);
        }
        if (i == 33)
        {
            windowSize = 300;
            SDL_SetWindowSize(window.get(),windowSize,windowSize);
        }
        else if (i == 66)
        {
            windowSize = 75;
            SDL_SetWindowSize(window.get(),windowSize,windowSize);
        }
        GTEST_ASSERT_TRUE(swapchain->backBufferWidth() == windowSize);
        GTEST_ASSERT_TRUE(swapchain->backBufferHeight() == windowSize);
    }

    swapchain->backBufferSize(1920,1080);
    GTEST_ASSERT_EQ(swapchain->backBufferWidth(),1920);
    GTEST_ASSERT_EQ(swapchain->backBufferHeight(),1080);
    GTEST_ASSERT_EQ(swapchain->currentFrame()->backBuffer()->width(),1920);
    GTEST_ASSERT_EQ(swapchain->currentFrame()->backBuffer()->height(),1080);

}

TEST(SwapChain, Format)
{
    auto window = utilities::createWindow("Format",150,150);
    auto swapchain = utilities::createSwapChain(window.get(),1,SwapChain::PresentMode::IMMEDIATE,Pixels::Format::B8G8R8A8_UNORM_SRGB,createResources);
    GTEST_ASSERT_EQ(swapchain->backBufferFormat(),Pixels::Format::B8G8R8A8_UNORM_SRGB);
    GTEST_ASSERT_EQ(swapchain->currentFrame()->backBuffer()->format(),Pixels::Format::B8G8R8A8_UNORM_SRGB);
    swapchain->backBufferFormat(Pixels::Format::B8G8R8A8_UNORM);
    GTEST_ASSERT_EQ(swapchain->backBufferFormat(),Pixels::Format::B8G8R8A8_UNORM);
    GTEST_ASSERT_EQ(swapchain->currentFrame()->backBuffer()->format(),Pixels::Format::B8G8R8A8_UNORM);
}
TEST(SwapChain, SetProperties)
{
    auto window = utilities::createWindow("Set Properties",150,250);
    auto swapchain = utilities::createSwapChain(window.get(),1,SwapChain::PresentMode::IMMEDIATE,Pixels::Format::B8G8R8A8_UNORM_SRGB,createResources);
    auto backBufferCount = swapchain->framesCount();
    GTEST_ASSERT_EQ(swapchain->backBufferWidth(),150);
    GTEST_ASSERT_EQ(swapchain->backBufferHeight(),250);
    GTEST_ASSERT_EQ(swapchain->framesCount(), 1);
    GTEST_ASSERT_EQ(swapchain->presentMode(),SwapChain::PresentMode::IMMEDIATE);
    GTEST_ASSERT_EQ(swapchain->backBufferFormat(),Pixels::Format::B8G8R8A8_UNORM_SRGB);
    GTEST_ASSERT_EQ(swapchain->currentFrame()->backBuffer()->format(),Pixels::Format::B8G8R8A8_UNORM_SRGB);
    swapchain->setProperties(100,200,3,SwapChain::PresentMode::BUFFER,Pixels::Format::B8G8R8A8_UNORM,SwapChain::AlphaCompositing::IGNORE_ALPHA);
    GTEST_ASSERT_EQ(swapchain->backBufferWidth(),100);
    GTEST_ASSERT_EQ(swapchain->backBufferHeight(),200);
    GTEST_ASSERT_EQ(swapchain->framesCount(), 3);
    GTEST_ASSERT_EQ(swapchain->presentMode(),SwapChain::PresentMode::BUFFER);
    GTEST_ASSERT_EQ(swapchain->backBufferFormat(),Pixels::Format::B8G8R8A8_UNORM);
    GTEST_ASSERT_EQ(swapchain->currentFrame()->backBuffer()->format(),Pixels::Format::B8G8R8A8_UNORM);
}
#ifdef SLAG_DEBUG
TEST(SwapChain, ForceSubmit)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");

    auto window = utilities::createWindow("Force Submit",150,150);
    auto swapchain = utilities::createSwapChain(window.get(),2,SwapChain::PresentMode::BUFFER,Pixels::Format::B8G8R8A8_UNORM_SRGB,createResources);

    swapchain->next();
    ASSERT_DEATH(swapchain->next(),"current frame must be submitted");
}
#endif