#include "gtest/gtest.h"
#include "slag/SlagLib.h"

using namespace slag;

TEST(BufferTest, Copy)
{
    std::vector<Buffer::Accessibility> accessibility{Buffer::CPU,Buffer::GPU,Buffer::CPU_AND_GPU};
    std::vector<Buffer::Usage> usages{Buffer::Usage::DATA_BUFFER,Buffer::Usage::VERTEX_BUFFER,Buffer::Usage::INDEX_BUFFER,Buffer::Usage::STORAGE_BUFFER,Buffer::Usage::INDIRECT_BUFFER};
    for(auto from=0; from< accessibility.size(); from++)
    {
        Buffer::Accessibility fromAccess = accessibility[from];
        for(auto to=0; to<accessibility.size(); to++)
        {
            Buffer::Accessibility toAccess = accessibility[to];
            for(auto usage=0; usage<usages.size();usage++)
            {
                auto use = usages[usage];

                std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GpuQueue::TRANSFER));
                commandBuffer->begin();
                std::vector<Color> rawData{Color(1.0f,0.0f,0.0f,1.0f),Color(.7f,.6f,.2f,1.0f),Color(0.0f,.9f,.25f,.33f)};
                std::unique_ptr<Buffer> buffer1 = std::unique_ptr<Buffer>(Buffer::newBuffer(rawData.data(), sizeof(Color) * rawData.size(), fromAccess, use));
                std::unique_ptr<Buffer> buffer2 = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(Color) * rawData.size(), toAccess, use));
                commandBuffer->copyBuffer(buffer1.get(), 0, buffer1->size(), buffer2.get(), 0);
                commandBuffer->end();
                SlagLib::graphicsCard()->transferQueue()->submit(commandBuffer.get());
                commandBuffer->waitUntilFinished();
                auto data = buffer2->downloadData();

                std::vector<Color> processedData(rawData.size());
                memcpy(processedData.data(),data.data(),data.size());
                for(int i=0; i<rawData.size(); i++)
                {
                    auto original = rawData[i];
                    auto proccessed = processedData[i];
                    GTEST_ASSERT_TRUE(original == proccessed);
                }
            }

        }
    }

}

TEST(BufferTest, CpuHandle)
{
    auto gpuBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(64,Buffer::GPU,Buffer::DATA_BUFFER));
    auto cpuBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(64,Buffer::CPU,Buffer::DATA_BUFFER));
    auto bothBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(64,Buffer::CPU_AND_GPU,Buffer::DATA_BUFFER));
    GTEST_ASSERT_EQ(gpuBuffer->cpuHandle(),nullptr);
    GTEST_ASSERT_NE(cpuBuffer->cpuHandle(),nullptr);
    GTEST_ASSERT_NE(bothBuffer->cpuHandle(),nullptr);
}