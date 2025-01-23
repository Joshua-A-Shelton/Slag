#include "gtest/gtest.h"
#include "slag/SlagLib.h"
#include "../utils/Window.h"

using namespace slag;

class DefaultFrameResources: public slag::FrameResources
{
public:
    slag::CommandBuffer* commandBuffer = nullptr;
    DefaultFrameResources()
    {
        commandBuffer = CommandBuffer::newCommandBuffer(slag::GpuQueue::GRAPHICS);
    }
    ~DefaultFrameResources()override
    {
        delete(commandBuffer);
    }
    void waitForResourcesToFinish()override
    {
        commandBuffer->waitUntilFinished();
    }
    bool isFinished()override
    {
        return commandBuffer->isFinished();
    }
};

FrameResources* defaultResource(size_t index, Swapchain* from)
{
    return new DefaultFrameResources();
}

TEST(Swapchain, PresentModes)
{
    auto window = slag::Window::makeWindow("Swapchain Present Modes",500,500);
    auto swapchain = slag::Window::makeSwapchain(window.get(),3,Swapchain::PresentMode::MAILBOX,Pixels::Format::B8G8R8A8_UNORM,defaultResource);

    Uint64 totalStart = SDL_GetPerformanceCounter();
    Uint64 last = totalStart;
    for(int i=0; i< 300; i++)
    {
        if(auto frame = swapchain->next())
        {
            auto cb = static_cast<DefaultFrameResources*>(frame->resources)->commandBuffer;
            cb->begin();
            cb->clearColorImage(frame->backBuffer(), {.floats={i/300.0f, 0, 0, 1}}, Texture::Layout::UNDEFINED, Texture::Layout::PRESENT,PipelineStageFlags::NONE,PipelineStageFlags::ALL_GRAPHICS);
            cb->end();
            SlagLib::graphicsCard()->graphicsQueue()->submit(&cb,1, nullptr,0, nullptr,0,frame);
        }
    }
    Uint64 totalEnd = SDL_GetPerformanceCounter();
    auto discardTime = ((totalEnd-totalStart)/1000)/300;

    swapchain->presentMode(Swapchain::PresentMode::FIFO);

    totalStart = SDL_GetPerformanceCounter();
    for(int i=0; i< 300; i++)
    {
        if(auto frame = swapchain->next())
        {
            auto cb = static_cast<DefaultFrameResources*>(frame->resources)->commandBuffer;
            cb->begin();
            cb->clearColorImage(frame->backBuffer(), {.floats={i/300.0f, 0, i/300.0f, 1}}, Texture::Layout::UNDEFINED, Texture::Layout::PRESENT,PipelineStageFlags::NONE,PipelineStageFlags::ALL_GRAPHICS);
            cb->end();
            SlagLib::graphicsCard()->graphicsQueue()->submit(&cb,1, nullptr,0, nullptr,0,frame);

        }
    }
    totalEnd = SDL_GetPerformanceCounter();
    auto sequentialTime = ((totalEnd-totalStart)/1000)/300;
    GTEST_ASSERT_GE(sequentialTime,(discardTime*1.05));
}

TEST(Swapchain, NextIfReady)
{
    auto window = slag::Window::makeWindow("Swapchain Next If Ready",500,500);
    auto swapchain = slag::Window::makeSwapchain(window.get(),3,Swapchain::PresentMode::MAILBOX,Pixels::Format::B8G8R8A8_UNORM,defaultResource);

    int frameCount = 0;
    int i=0;
    for(;;)
    {
        i++;
        if(auto frame = swapchain->nextIfReady())
        {
            auto cb = static_cast<DefaultFrameResources*>(frame->resources)->commandBuffer;
            cb->begin();
            cb->clearColorImage(frame->backBuffer(), {.floats={0, 1, 0, 1}}, Texture::Layout::UNDEFINED, Texture::Layout::PRESENT,PipelineStageFlags::NONE,PipelineStageFlags::ALL_GRAPHICS);
            cb->end();
            SlagLib::graphicsCard()->graphicsQueue()->submit(&cb,1, nullptr,0, nullptr,0,frame);

            frameCount++;
        }
        if(frameCount > 16 || i == INT_MAX)
        {
            break;
        }
    }
    int j=0;
    int frameCount2=0;
    for(;; j++)
    {
        if(auto frame = swapchain->next())
        {
            auto cb = static_cast<DefaultFrameResources*>(frame->resources)->commandBuffer;
            cb->begin();
            cb->clearColorImage(frame->backBuffer(), {.floats={0, 0, 1, 1}}, Texture::Layout::UNDEFINED, Texture::Layout::PRESENT,PipelineStageFlags::NONE,PipelineStageFlags::ALL_GRAPHICS);
            cb->end();
            SlagLib::graphicsCard()->graphicsQueue()->submit(&cb,1, nullptr,0, nullptr,0,frame);
            frameCount2++;
        }
        if(frameCount2 >16 || j == INT_MAX)
        {
            break;
        }
    }

    GTEST_ASSERT_GE(i,frameCount);
    GTEST_ASSERT_EQ(j,frameCount2-1);
}