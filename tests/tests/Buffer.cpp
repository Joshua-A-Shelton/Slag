#include <gtest/gtest.h>
#include <slag/Slag.h>

using namespace slag;

class BufferTest : public ::testing::TestWithParam<Buffer*>
{
public:
    std::unique_ptr<Buffer> buffer0;
    std::unique_ptr<Buffer> buffer1;
    std::unique_ptr<Buffer> buffer2;
    std::unique_ptr<Buffer> buffer3;
    std::unique_ptr<Buffer> buffer4;
    std::unique_ptr<Buffer> buffer5;
    std::unique_ptr<Buffer> buffer6;
    std::unique_ptr<Buffer> buffer7;

    std::unique_ptr<Buffer> buffer8;
    std::unique_ptr<Buffer> buffer9;
    std::unique_ptr<Buffer> buffer10;
    std::unique_ptr<Buffer> buffer11;
    std::unique_ptr<Buffer> buffer12;
    std::unique_ptr<Buffer> buffer13;
    std::unique_ptr<Buffer> buffer14;
    std::unique_ptr<Buffer> buffer15;

    std::vector<Buffer*> cpuBuffers;
    std::vector<Buffer*> gpuBuffers;
    std::vector<Buffer*> dataBuffers;
    std::vector<Buffer*> indexBuffers;
    std::vector<Buffer*> indirectBuffers;
    std::vector<Buffer*> storageBuffers;
    std::vector<Buffer*> storageTexelBuffers;
    std::vector<Buffer*> uniformBuffers;
    std::vector<Buffer*> uniformTexelBuffers;
    std::vector<Buffer*> vertexBuffers;
    std::vector<Buffer*> preInitializedBuffers;

    BufferTest()
    {
        std::vector<uint8_t> rawData(200,128);
        buffer0 = std::unique_ptr<Buffer>(Buffer::newBuffer(200,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::DATA_BUFFER));
        buffer1 = std::unique_ptr<Buffer>(Buffer::newBuffer(rawData.data(),200,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::INDEX_BUFFER));
        buffer2 = std::unique_ptr<Buffer>(Buffer::newBuffer(200,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::INDIRECT_BUFFER));
        buffer3 = std::unique_ptr<Buffer>(Buffer::newBuffer(rawData.data(),200,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::STORAGE_BUFFER));
        buffer4 = std::unique_ptr<Buffer>(Buffer::newBuffer(200,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::STORAGE_TEXEL_BUFFER));
        buffer5 = std::unique_ptr<Buffer>(Buffer::newBuffer(rawData.data(),200,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_BUFFER));
        buffer6 = std::unique_ptr<Buffer>(Buffer::newBuffer(200,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_TEXEL_BUFFER));
        buffer7 = std::unique_ptr<Buffer>(Buffer::newBuffer(rawData.data(),200,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::VERTEX_BUFFER));

        buffer8 = std::unique_ptr<Buffer>(Buffer::newBuffer(rawData.data(),200,Buffer::Accessibility::GPU,Buffer::UsageFlags::DATA_BUFFER));
        buffer9 = std::unique_ptr<Buffer>(Buffer::newBuffer(200,Buffer::Accessibility::GPU,Buffer::UsageFlags::INDEX_BUFFER));
        buffer10 = std::unique_ptr<Buffer>(Buffer::newBuffer(rawData.data(),200,Buffer::Accessibility::GPU,Buffer::UsageFlags::INDIRECT_BUFFER));
        buffer11 = std::unique_ptr<Buffer>(Buffer::newBuffer(200,Buffer::Accessibility::GPU,Buffer::UsageFlags::STORAGE_BUFFER));
        buffer12 = std::unique_ptr<Buffer>(Buffer::newBuffer(rawData.data(),200,Buffer::Accessibility::GPU,Buffer::UsageFlags::STORAGE_TEXEL_BUFFER));
        buffer13 = std::unique_ptr<Buffer>(Buffer::newBuffer(200,Buffer::Accessibility::GPU,Buffer::UsageFlags::UNIFORM_BUFFER));
        buffer14 = std::unique_ptr<Buffer>(Buffer::newBuffer(rawData.data(),200,Buffer::Accessibility::GPU,Buffer::UsageFlags::UNIFORM_TEXEL_BUFFER));
        buffer15 = std::unique_ptr<Buffer>(Buffer::newBuffer(200,Buffer::Accessibility::GPU,Buffer::UsageFlags::VERTEX_BUFFER));

        cpuBuffers.push_back(buffer0.get());
        cpuBuffers.push_back(buffer1.get());
        cpuBuffers.push_back(buffer2.get());
        cpuBuffers.push_back(buffer3.get());
        cpuBuffers.push_back(buffer4.get());
        cpuBuffers.push_back(buffer5.get());
        cpuBuffers.push_back(buffer6.get());
        cpuBuffers.push_back(buffer7.get());

        gpuBuffers.push_back(buffer8.get());
        gpuBuffers.push_back(buffer9.get());
        gpuBuffers.push_back(buffer10.get());
        gpuBuffers.push_back(buffer11.get());
        gpuBuffers.push_back(buffer12.get());
        gpuBuffers.push_back(buffer13.get());
        gpuBuffers.push_back(buffer14.get());
        gpuBuffers.push_back(buffer15.get());

        dataBuffers.push_back(buffer0.get());
        dataBuffers.push_back(buffer8.get());

        indexBuffers.push_back(buffer1.get());
        indexBuffers.push_back(buffer9.get());

        indirectBuffers.push_back(buffer2.get());
        indirectBuffers.push_back(buffer10.get());

        storageBuffers.push_back(buffer3.get());
        storageBuffers.push_back(buffer11.get());

        storageTexelBuffers.push_back(buffer4.get());
        storageTexelBuffers.push_back(buffer12.get());

        uniformBuffers.push_back(buffer5.get());
        uniformBuffers.push_back(buffer13.get());

        uniformTexelBuffers.push_back(buffer6.get());
        uniformTexelBuffers.push_back(buffer14.get());

        vertexBuffers.push_back(buffer7.get());
        vertexBuffers.push_back(buffer15.get());

        preInitializedBuffers.push_back(buffer1.get());
        preInitializedBuffers.push_back(buffer3.get());
        preInitializedBuffers.push_back(buffer5.get());
        preInitializedBuffers.push_back(buffer7.get());
        preInitializedBuffers.push_back(buffer8.get());
        preInitializedBuffers.push_back(buffer10.get());
        preInitializedBuffers.push_back(buffer12.get());
        preInitializedBuffers.push_back(buffer14.get());
    }
};

TEST_F(BufferTest,IsCPUVisible)
{
    for (auto& cpuBuffer : cpuBuffers)
    {
        GTEST_ASSERT_EQ(cpuBuffer->accessibility(),Buffer::Accessibility::CPU_AND_GPU);
    }
}


TEST_F(BufferTest,IsGPUVisible)
{
    for (auto& gpuBuffer : gpuBuffers)
    {
        GTEST_ASSERT_EQ(gpuBuffer->accessibility(),Buffer::Accessibility::GPU);
    }
}

TEST_F(BufferTest, IsPreInitialized)
{
    for (auto& buffer : preInitializedBuffers)
    {
        std::unique_ptr<Buffer> download = std::unique_ptr<Buffer>(Buffer::newBuffer(200,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::DATA_BUFFER));
        std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::TRANSFER));
        std::unique_ptr<Semaphore> semaphore = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));

        commandBuffer->begin();

        commandBuffer->copyBufferToBuffer(buffer,0,download->size(),download.get(),0);

        commandBuffer->end();

        SemaphoreValue value{semaphore.get(),1};
        auto cmdBufferPtr = commandBuffer.get();
        QueueSubmissionBatch batch
        {
            .waitSemaphores = nullptr,
            .waitSemaphoreCount = 0,
            .commandBuffers = &cmdBufferPtr,
            .commandBufferCount = 1,
            .signalSemaphores = &value,
            .signalSemaphoreCount = 1,
        };

        slagGraphicsCard()->transferQueue()->submit(&batch,1);
        semaphore->waitForValue(1);
        uint8_t* results = download->as<uint8_t>();
        for (int i=0; i<200; i++)
        {
            GTEST_ASSERT_EQ(results[i],128);
        }

    }
}

TEST_F(BufferTest, UsageDataBuffer)
{
    for (auto& buffer : dataBuffers)
    {
        GTEST_ASSERT_EQ(buffer->usage(),Buffer::UsageFlags::DATA_BUFFER);
    }
}
TEST_F(BufferTest, UsageIndexBuffer)
{
    for (auto& buffer : indexBuffers)
    {
        GTEST_ASSERT_EQ(buffer->usage(),Buffer::UsageFlags::INDEX_BUFFER);
    }
}
TEST_F(BufferTest, UsageIndirect)
{
    for (auto& buffer : indirectBuffers)
    {
        GTEST_ASSERT_EQ(buffer->usage(),Buffer::UsageFlags::INDIRECT_BUFFER);
    }
}
TEST_F(BufferTest, UsageStorageBuffer)
{
    for (auto& buffer : storageBuffers)
    {
        GTEST_ASSERT_EQ(buffer->usage(),Buffer::UsageFlags::STORAGE_BUFFER);
    }
}
TEST_F(BufferTest, UsageStorageTexelBuffer)
{
    for (auto& buffer : storageTexelBuffers)
    {
        GTEST_ASSERT_EQ(buffer->usage(),Buffer::UsageFlags::STORAGE_TEXEL_BUFFER);
    }
}
TEST_F(BufferTest, UsageUniformBuffer)
{
    for (auto& buffer : uniformBuffers)
    {
        GTEST_ASSERT_EQ(buffer->usage(),Buffer::UsageFlags::UNIFORM_BUFFER);
    }
}
TEST_F(BufferTest, UsageUniformTexelBuffer)
{
    for (auto& buffer : uniformTexelBuffers)
    {
        GTEST_ASSERT_EQ(buffer->usage(),Buffer::UsageFlags::UNIFORM_TEXEL_BUFFER);
    }
}
TEST_F(BufferTest, UsageVertexBuffer)
{
    for (auto& buffer : vertexBuffers)
    {
        GTEST_ASSERT_EQ(buffer->usage(),Buffer::UsageFlags::VERTEX_BUFFER);
    }
}

TEST_F(BufferTest, CPUHandlesForCPUVisible)
{
    for (auto& cpuBuffer : cpuBuffers)
    {
        GTEST_ASSERT_NE(cpuBuffer->cpuHandle(),nullptr);
    }
}
#ifdef SLAG_DEBUG
TEST_F(BufferTest, CPUHandlesForGPUVisible)
{
    for (auto& gpuBuffer : gpuBuffers)
    {
        EXPECT_DEATH(gpuBuffer->cpuHandle(),"Buffer must be CPU accessible");
    }
}
#endif

TEST_F(BufferTest, CountAsArray)
{
    std::unique_ptr<Buffer> buffer1 = std::unique_ptr<Buffer>(Buffer::newBuffer(256,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::DATA_BUFFER));
    GTEST_ASSERT_EQ(buffer1->size(),256);
    GTEST_ASSERT_EQ(buffer1->countAsArray<uint8_t>(),256);
    GTEST_ASSERT_EQ(buffer1->countAsArray<uint16_t>(),128);
    GTEST_ASSERT_EQ(buffer1->countAsArray<float>(),64);

    std::unique_ptr<Buffer> buffer2 = std::unique_ptr<Buffer>(Buffer::newBuffer(255,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::DATA_BUFFER));
    GTEST_ASSERT_EQ(buffer2->size(),255);
    GTEST_ASSERT_EQ(buffer2->countAsArray<uint8_t>(),255);
    GTEST_ASSERT_EQ(buffer2->countAsArray<uint16_t>(),255/sizeof(uint16_t));
    GTEST_ASSERT_EQ(buffer2->countAsArray<float>(),255/(sizeof(float)));

}

TEST_F(BufferTest, UpdateCPUBuffer)
{
    std::vector<uint8_t> initData(256,0);
    std::unique_ptr<Buffer> dataBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(initData.data(),256,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::DATA_BUFFER));
    std::vector<uint8_t> newData(256);
    for (int i = 0; i < 256; i++)
    {
        newData[i] = i;
    }

    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::TRANSFER));
    std::unique_ptr<Semaphore> commandsFinishedSemaphore = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    std::unique_ptr<Semaphore> updateFinishedSemaphore = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    std::unique_ptr<Buffer> junkBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(uint32_t)*1920*1080*4,Buffer::Accessibility::GPU,Buffer::UsageFlags::DATA_BUFFER));

    commandBuffer->begin();
    commandBuffer->fillBuffer(junkBuffer.get(),0,junkBuffer->size(),128453281);
    commandBuffer->end();

    auto cmdPtr = commandBuffer.get();
    SemaphoreValue signal
    {
        .semaphore = commandsFinishedSemaphore.get(),
        .value = 1
    };
    SemaphoreValue signal2
    {
        .semaphore = updateFinishedSemaphore.get(),
        .value = 1
    };
    QueueSubmissionBatch batch
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = &cmdPtr,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1,
    };
    slagGraphicsCard()->transferQueue()->submit(&batch,1);

    dataBuffer->update(32,newData.data()+32,128,&signal,1,&signal2,1);
    //update is blocking, no need to wait, just make sure it fired the value change
    GTEST_ASSERT_EQ(updateFinishedSemaphore->value(),1);

    auto data = dataBuffer->as<uint8_t>();
    for (auto i=0; i<dataBuffer->countAsArray<uint8_t>(); i++)
    {
        if (32 <= i && i < 32+128)
        {
            GTEST_ASSERT_EQ(data[i],i);
        }
        else
        {
            GTEST_ASSERT_EQ(data[i],0);
        }
    }
}

TEST_F(BufferTest, UpdateGPUBuffer)
{
    std::vector<uint8_t> initData(256,0);
    std::unique_ptr<Buffer> dataBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(initData.data(),256,Buffer::Accessibility::GPU,Buffer::UsageFlags::DATA_BUFFER));
    std::vector<uint8_t> newData(256);
    for (int i = 0; i < 256; i++)
    {
        newData[i] = i;
    }

    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::TRANSFER));
    std::unique_ptr<Semaphore> commandsFinishedSemaphore = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    std::unique_ptr<Semaphore> updateFinishedSemaphore = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    std::unique_ptr<Buffer> junkBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(uint32_t)*1920*1080*4,Buffer::Accessibility::GPU,Buffer::UsageFlags::DATA_BUFFER));

    commandBuffer->begin();
    commandBuffer->fillBuffer(junkBuffer.get(),0,junkBuffer->size(),128453281);
    commandBuffer->end();

    auto cmdPtr = commandBuffer.get();
    SemaphoreValue signal
    {
        .semaphore = commandsFinishedSemaphore.get(),
        .value = 1
    };
    SemaphoreValue signal2
    {
        .semaphore = updateFinishedSemaphore.get(),
        .value = 1
    };
    QueueSubmissionBatch batch
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = &cmdPtr,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1
    };
    slagGraphicsCard()->transferQueue()->submit(&batch,1);

    dataBuffer->update(32,newData.data()+32,128,&signal,1,&signal2,1);
    //update is blocking, no need to wait, just make sure it fired the value change
    GTEST_ASSERT_EQ(updateFinishedSemaphore->value(),1);

    auto download = std::unique_ptr<Buffer>(Buffer::newBuffer(dataBuffer->size(),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::DATA_BUFFER));
    commandBuffer->begin();
    commandBuffer->copyBufferToBuffer(dataBuffer.get(),0,download->size(),download.get(),0);
    commandBuffer->end();

    signal.value = 2;
    slagGraphicsCard()->transferQueue()->submit(&batch,1);
    commandsFinishedSemaphore->waitForValue(2);

    auto data = download->as<uint8_t>();
    for (auto i=0; i<download->countAsArray<uint8_t>(); i++)
    {
        if (32 <= i && i <32+128)
        {
            GTEST_ASSERT_EQ(data[i],i);
        }
        else
        {
            GTEST_ASSERT_EQ(data[i],0);
        }
    }
}
