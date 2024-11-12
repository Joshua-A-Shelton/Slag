#include "gtest/gtest.h"
#include "slag/SlagLib.h"

using namespace slag;

TEST(BufferTest, Copy)
{
    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GpuQueue::Transfer));
    commandBuffer->begin();
    std::vector<Color> rawData{Color(1.0f,0.0f,0.0f,1.0f),Color(.7f,.6f,.2f,1.0f),Color(0.0f,.9f,.25f,.33f)};
    std::unique_ptr<Buffer> cpuBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(rawData.data(), sizeof(Color) * rawData.size(), Buffer::Accessibility::CPU, Buffer::Usage::DATA_BUFFER));
    std::unique_ptr<Buffer> gpuBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(Color) * rawData.size(), Buffer::Accessibility::GPU, Buffer::Usage::DATA_BUFFER));
    commandBuffer->copyBuffer(cpuBuffer.get(),0,cpuBuffer->size(),gpuBuffer.get(),0);
    commandBuffer->end();
    SlagLib::graphicsCard()->transferQueue()->submit(commandBuffer.get());
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
}