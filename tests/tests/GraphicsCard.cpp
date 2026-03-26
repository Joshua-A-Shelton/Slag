#include <gtest/gtest.h>
#include <slag/Slag.h>

#include "../third-party/LodePNG/lodepng.h"
using namespace slag;
TEST(GraphicsCard, Name)
{
    GTEST_ASSERT_TRUE(Slag::backend()->graphicsCardCount() > 0);
    for (auto i=0u; i< Slag::backend()->graphicsCardCount(); i++)
    {
        GraphicsCard* card = Slag::backend()->graphicsCard(i);
        GTEST_ASSERT_TRUE(card->name().length()>1);
    }
}
TEST(GraphicsCard, VideoMemory)
{
    GTEST_ASSERT_TRUE(Slag::backend()->graphicsCardCount() > 0);
    for (auto i=0u; i< Slag::backend()->graphicsCardCount(); i++)
    {
        GraphicsCard* card = Slag::backend()->graphicsCard(i);
        GTEST_ASSERT_TRUE(card->memoryProperties().videoMemory>0);
    }
}

TEST(GraphicsCard, MaxShaderAccessReadOnlyBufferSize)
{
    GTEST_ASSERT_TRUE(Slag::backend()->graphicsCardCount() > 0);
    for (auto i=0u; i< Slag::backend()->graphicsCardCount(); i++)
    {
        GraphicsCard* card = Slag::backend()->graphicsCard(i);
        GTEST_ASSERT_TRUE(card->memoryProperties().maxUniformBufferSize>=16384);
    }
}

TEST(GraphicsCard, Defragment)
{
    bool didTest = false;
    for (auto i=0u; i< Slag::backend()->graphicsCardCount(); i++)
    {
        auto card = Slag::backend()->graphicsCard(i);
        if (!card->capabilities().defragmentable)
        {
            continue;
        }
        didTest = true;
        auto buffer1 = std::unique_ptr<Buffer>(card->newBuffer(256));
        auto buffer2 = std::unique_ptr<Buffer>(card->newBuffer(256));
        auto buffer3 = std::unique_ptr<Buffer>(card->newBuffer(256*500));
        auto buffer4 = std::unique_ptr<Buffer>(card->newBuffer(128*1024));
        auto buffer5 = std::unique_ptr<Buffer>(card->newBuffer(256*500));
        auto buffer6 = std::unique_ptr<Buffer>(card->newBuffer(512));
        auto buffer7 = std::unique_ptr<Buffer>(card->newBuffer(512*1024));
        auto buffer8 = std::unique_ptr<Buffer>(card->newBuffer(256*500));
        auto buffer9 = std::unique_ptr<Buffer>(card->newBuffer(128*1024));
        auto buffer10 = std::unique_ptr<Buffer>(card->newBuffer(128*1024));
        auto buffer11 = std::unique_ptr<Buffer>(card->newBuffer(128*1024));
        //need to do something here to see that it's moved... it should be returned as an object that's been moved
        auto texture1 = std::unique_ptr<Texture>(card->newTexture2D(500,500,PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::SAMPLED));
        auto texture2 = std::unique_ptr<Texture>(card->newTexture2D(500,500,PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::SAMPLED));
        auto texture3 = std::unique_ptr<Texture>(card->newTexture2D(250,250,PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::SAMPLED));
        auto texture4 = std::unique_ptr<Texture>(card->newTexture2D(1500,1500,PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::SAMPLED));
        auto texture5 = std::unique_ptr<Texture>(card->newTexture2D(250,250,PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::SAMPLED));
        auto texture6 = std::unique_ptr<Texture>(card->newTexture2D(750,750,PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::SAMPLED));
        auto uploadBuffer = std::unique_ptr<Buffer>(card->newBuffer(256,BufferCPUAccess::READ_WRITE,BufferMemoryType::UNIFORM));

        auto bufferSize = texture6->bufferSize(PixelAspect::COLOR);
        auto dataBuffer = std::unique_ptr<Buffer>(card->newBuffer(bufferSize,BufferCPUAccess::WRITE_ONLY,BufferMemoryType::GENERAL));
        auto dataBufferPtr = dataBuffer->as<uint8_t>();
        for (uint64_t i=0u; i<bufferSize; i+=4)
        {
            dataBufferPtr[i] = 255;
            dataBufferPtr[i+1] = 172;
            dataBufferPtr[i+2] = 0;
            dataBufferPtr[i+3] = 255;
        }
        auto data = uploadBuffer->as<uint8_t>();
        for (int i=0; i< 256; i++)
        {
            data[i] = i;
        }
        auto finished = std::unique_ptr<Semaphore>(card->newSemaphore());

        auto virtualAddress = buffer8->deviceAddress();

        auto commandBuffer = std::unique_ptr<CommandBuffer>(card->newCommandBuffer(QueueType::TRANSFER));
        commandBuffer->begin();
        commandBuffer->copyBufferToBuffer(uploadBuffer.get(),0,buffer8.get(),0,256);
        TextureBufferMapping mapping
        {
            .bufferOffset = 0,
            .subresource =
            {
                .aspect = PixelAspect::COLOR,
                .mipLevel = 0,
                .baseArrayLayer =0,
                .layerCount = 1
            },
            .offset = {0,0,0},
            .extent = {texture6->width(),texture6->height(),1}
        };
        commandBuffer->copyBufferToTexture(dataBuffer.get(),texture6.get(),&mapping,1);
        commandBuffer->end();

        SemaphoreValue signal
        {
            .semaphore = finished.get(),
            .value = 1
        };
        auto cbuffer = commandBuffer.get();
        SubmissionBatch batch
        {
            .waitSemaphores = nullptr,
            .waitSemaphoreCount = 0,
            .commandBuffers = &cbuffer,
            .commandBufferCount = 1,
            .signalSemaphores = &signal,
            .signalSemaphoreCount = 1,
        };

        card->transferQueue()->submit(&batch,1);
        finished->waitForValue(1);
        buffer2 = nullptr;
        buffer3 = nullptr;
        //buffer4 = nullptr;
        buffer5 = nullptr;
        //buffer6 = nullptr;
        buffer7 = nullptr;
        texture1 = nullptr;
        texture2 = nullptr;
        texture3 = nullptr;
        texture4 = nullptr;
        texture5 = nullptr;
        dataBuffer = nullptr;
        GTEST_ASSERT_EQ(virtualAddress, buffer8->deviceAddress());

        std::vector<Texture*> movedTextures;
        std::vector<Buffer*> movedBuffers;
        auto bytes = card->defragmentMemory(0,[&movedTextures, &movedBuffers](MemoryReference* memoryReference)
        {
            if (memoryReference->type == MemoryObjectType::TEXTURE)
            {
                movedTextures.push_back(memoryReference->memory.texture);
            }
            else
            {
                movedBuffers.push_back(memoryReference->memory.buffer);
            }
        });
        GTEST_ASSERT_GT(bytes,0);
        GTEST_ASSERT_NE(virtualAddress, buffer8->deviceAddress());
        GTEST_ASSERT_GE(movedTextures.size(), 1);
        GTEST_ASSERT_GE(movedBuffers.size(),1);

        auto downloadData = std::unique_ptr<Buffer>(card->newBuffer(256,BufferCPUAccess::READ_WRITE));
        auto transferFinished = std::unique_ptr<Semaphore>(card->newSemaphore());
        auto downloadBuffer = std::unique_ptr<Buffer>(card->newBuffer(bufferSize,BufferCPUAccess::READ_WRITE));
        commandBuffer->begin();
        commandBuffer->copyBufferToBuffer(buffer8.get(),0,downloadData.get(),0,256);
        commandBuffer->copyTextureToBuffer(texture6.get(),downloadBuffer.get(),&mapping,1);
        commandBuffer->end();

        signal.semaphore = transferFinished.get();
        cbuffer = commandBuffer.get();
        card->transferQueue()->submit(&batch,1);
        transferFinished->waitForValue(1);
        data = downloadData->as<uint8_t>();
        for (int i=0; i< 256; i++)
        {
            GTEST_ASSERT_EQ(data[i], i);
        }

        auto textureData = downloadBuffer->as<uint8_t>();
        for (uint64_t i=0; i< bufferSize; i+=4)
        {
            GTEST_ASSERT_EQ(textureData[i], 255);
            GTEST_ASSERT_EQ(textureData[i+1], 172);
            GTEST_ASSERT_EQ(textureData[i+2], 0);
            GTEST_ASSERT_EQ(textureData[i+3], 255);
        }
    }
    if (!didTest)
    {
        GTEST_SKIP();
    }
}