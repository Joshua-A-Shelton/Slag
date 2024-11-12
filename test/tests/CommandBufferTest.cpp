#include "gtest/gtest.h"
#include "slag/SlagLib.h"
using namespace slag;

TEST(CommandBuffer, StartFinished)
{
    std::unique_ptr<CommandBuffer> buffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GpuQueue::Graphics));
    ASSERT_TRUE(buffer->isFinished());
}