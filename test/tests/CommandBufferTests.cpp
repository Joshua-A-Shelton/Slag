#include <gtest/gtest.h>
#include <slag/SlagLib.h>
using namespace slag;

TEST(CommandBuffer, Creation)
{
    auto buffer = CommandBuffer::newCommandBuffer(GpuQueue::Graphics);
    delete buffer;
}