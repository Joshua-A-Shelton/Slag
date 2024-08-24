#include "gtest/gtest.h"
#include "slag/SlagLib.h"
using namespace slag;

TEST(CommandBuffer, Creation)
{
    auto buffer = CommandBuffer::newCommandBuffer(GpuQueue::Graphics);
    delete buffer;
    int i=0;
}

TEST(CommandBuffer, ReplaceMe)
{
    auto buffer = CommandBuffer::newCommandBuffer(GpuQueue::Graphics);
    ASSERT_TRUE(buffer->isFinished());
    delete buffer;
}