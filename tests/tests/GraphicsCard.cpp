#include <gtest/gtest.h>
#include <slag/Slag.h>

#include "../third-party/LodePNG/lodepng.h"

using namespace slag;

TEST(GraphicsCard, Exists)
{
    GTEST_ASSERT_NE(slagGraphicsCard(),nullptr);
}

TEST(GraphicsCard, VideoMemory)
{
    GTEST_ASSERT_GT(slagGraphicsCard()->videoMemory(),0);
}

TEST(GraphicsCard, ProvidesQueue)
{
    GTEST_ASSERT_NE(slagGraphicsCard()->graphicsQueue(),nullptr);
    GTEST_ASSERT_NE(slagGraphicsCard()->computeQueue(),nullptr);
    GTEST_ASSERT_NE(slagGraphicsCard()->transferQueue(),nullptr);
}

TEST(GraphicsCard, UniformBufferOffsetAlignment)
{
    GTEST_ASSERT_GE(slagGraphicsCard()->uniformBufferOffsetAlignment(),0);
}

TEST(GraphicsCard, StorageBufferOffsetAlignment)
{
    GTEST_ASSERT_GE(slagGraphicsCard()->storageBufferOffsetAlignment(),0);
}

TEST(GraphicsCard, DefragmentMemory)
{
    auto tex1 = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::SAMPLED_IMAGE,50,50,1,3,5));
    class uint8Pixel
    {
    public:
        uint8_t r,g,b,a;
    };
    std::vector<uint8Pixel> pixels(tex1->byteSize()/sizeof(uint8Pixel));
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    uint8_t alpha = 255;
    for (size_t i=0; i<pixels.size(); i++)
    {
        if (red == 255)
        {
            if (green == 255)
            {
                if (blue == 255)
                {
                    red = 0;
                    green = 0;
                    blue = 0;
                }
                else
                {
                    blue++;
                }
            }
            else
            {
                green++;
            }
        }
        else
        {
            red++;
        }
        pixels[i].r = red;
        pixels[i].g = green;
        pixels[i].b = blue;
        pixels[i].a = alpha;
    }

    TextureBufferMapping mappings[15];
    size_t offset = 0;
    for (auto arrayLevel = 0; arrayLevel<5; arrayLevel++)
    {
        for (auto mipLevel=0; mipLevel<3; mipLevel++)
        {
            auto mappingLevel = 3*arrayLevel + mipLevel;
            auto& mapping = mappings[mappingLevel];
            mapping.bufferOffset = offset;
            mapping.textureExtent = {tex1->width(mipLevel),tex1->height(mipLevel),1};
            mapping.textureOffset = {0,0,0};
            mapping.textureSubresource.aspectFlags = Pixels::AspectFlags::COLOR;
            mapping.textureSubresource.mipLevel = mipLevel;
            mapping.textureSubresource.baseArrayLayer = arrayLevel;
            mapping.textureSubresource.layerCount = 1;

            offset+= tex1->byteSize(mipLevel);
        }
    }


    auto tex2 = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::SAMPLED_IMAGE,50,50,1,3,5,Texture::SampleCount::ONE,pixels.data(),pixels.size()*sizeof(uint8Pixel),mappings,15));
    auto buf1 = std::unique_ptr<Buffer>(Buffer::newBuffer(64*64*64,Buffer::Accessibility::CPU_AND_GPU));
    //I have to use the buffer somewhere, or it can get optimized away
    if (buf1 == nullptr)
    {
        GTEST_FAIL();
    }
    auto afterBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(pixels.data(),pixels.size()*sizeof(uint8Pixel),Buffer::Accessibility::CPU_AND_GPU));
    tex1 = nullptr;
    buf1 = nullptr;
    slagGraphicsCard()->defragmentMemory(nullptr,0,nullptr,0);

    auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::TRANSFER));
    auto textureBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(tex2->byteSize(),Buffer::Accessibility::CPU_AND_GPU));
    commandBuffer->begin();
    commandBuffer->copyTextureToBuffer(tex2.get(),textureBuffer.get(),mappings,15);
    commandBuffer->end();

    auto finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    SemaphoreValue signal{.semaphore = finished.get(),.value =1};
    auto cbptr = commandBuffer.get();
    QueueSubmissionBatch batch
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = &cbptr,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1,
    };
    slagGraphicsCard()->transferQueue()->submit(&batch,1);
    finished->waitForValue(1);



    auto texPtr = textureBuffer->as<uint8Pixel>();
    for (auto i=0; i<pixels.size(); i++)
    {
        GTEST_ASSERT_EQ(texPtr[i].r,pixels[i].r);
        GTEST_ASSERT_EQ(texPtr[i].g,pixels[i].g);
        GTEST_ASSERT_EQ(texPtr[i].b,pixels[i].b);
        GTEST_ASSERT_EQ(texPtr[i].a,pixels[i].a);
    }

    auto pixelPtr = afterBuffer->as<uint8Pixel>();
    for (auto i=0; i<pixels.size(); i++)
    {
        GTEST_ASSERT_EQ(pixelPtr[i].r,pixels[i].r);
        GTEST_ASSERT_EQ(pixelPtr[i].g,pixels[i].g);
        GTEST_ASSERT_EQ(pixelPtr[i].b,pixels[i].b);
        GTEST_ASSERT_EQ(pixelPtr[i].a,pixels[i].a);
    }
}