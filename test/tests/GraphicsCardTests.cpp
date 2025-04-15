#include "gtest/gtest.h"
#include "slag/SlagLib.h"
using namespace slag;
TEST(GraphicsCard, Defrag)
{
    std::vector<std::unique_ptr<Buffer>> buffers(100);
    std::vector<unsigned char> rawData(256,128);
    std::vector<unsigned char> targetData(256,256);
    for(uint32_t i = 0; i < buffers.size()-1; ++i)
    {
        buffers[i] = std::unique_ptr<Buffer>(Buffer::newBuffer(rawData.data(), rawData.size(),slag::Buffer::CPU_AND_GPU,Buffer::DATA_BUFFER));
    }
    buffers[99] = std::unique_ptr<Buffer>(Buffer::newBuffer(targetData.data(), targetData.size(),slag::Buffer::CPU_AND_GPU,Buffer::DATA_BUFFER));
    for(int i = 0; i < buffers.size()-1; ++i)
    {
        buffers[i] = std::unique_ptr<Buffer>(nullptr);
    }
    auto& targetBuffer = buffers[99];
    auto memLocation = targetBuffer->

    GTEST_FAIL();
}