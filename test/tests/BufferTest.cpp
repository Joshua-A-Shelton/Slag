#include "gtest/gtest.h"
#include "slag/SlagLib.h"

using namespace slag;

TEST(BufferTest, Copy)
{
    CommandBuffer* commandBuffer = CommandBuffer::newCommandBuffer(GpuQueue::Transfer);
    commandBuffer->begin();
    std::vector<Color> rawData{Color(1.0f,0.0f,0.0f,1.0f),Color(.7f,.6f,.2f,1.0f),Color(0.0f,.9f,.25f,.33f)};
    auto cpuBuffer = Buffer::newBuffer(rawData.data(), sizeof(Color) * rawData.size(), Buffer::Accessibility::CPU, Buffer::Usage::DATA_BUFFER);
    auto gpuBuffer = Buffer::newBuffer(sizeof(Color) * rawData.size(), Buffer::Accessibility::GPU, Buffer::Usage::DATA_BUFFER);
    commandBuffer->copyBuffer(cpuBuffer,0,cpuBuffer->size(),gpuBuffer,0);
    commandBuffer->end();
    SlagLib::graphicsCard()->transferQueue()->submit(commandBuffer);
    commandBuffer->waitUntilFinished();
    auto data = gpuBuffer->downloadData();

    std::vector<Color> processedData(rawData.size());
    memcpy(processedData.data(),data.data(),data.size());
    for(int i=0; i<rawData.size(); i++)
    {
        auto original = rawData[i];
        auto proccessed = processedData[i];
        GTEST_ASSERT_TRUE(original == proccessed);
    }


    delete commandBuffer;
    delete cpuBuffer;
    delete gpuBuffer;
}