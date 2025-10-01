#include "slag/core/Texture.h"
#include "slag/core/Texture.h"

#include <gtest/gtest.h>
#include <slag/Slag.h>

#include "slag/core/PixelFormatProperties.h"
#include "slag/core/Pixels.h"
#include "slag/core/Pixels.h"

using namespace slag;

TEST(Texture, TextureSize2D)
{
    auto texture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R32G32B32A32_FLOAT,Texture::Type::TEXTURE_2D,Texture::UsageFlags::STORAGE,128,64,1,3,2));
    uint64_t mip0Size = Pixels::size(Pixels::Format::R32G32B32A32_FLOAT)*128*64;
    uint64_t mip1Size = Pixels::size(Pixels::Format::R32G32B32A32_FLOAT)*(128>>1)*(64>>1);
    uint64_t mip2Size = Pixels::size(Pixels::Format::R32G32B32A32_FLOAT)*(128>>2)*(64>>2);
    GTEST_ASSERT_EQ(texture->byteSize(),(mip0Size+mip1Size+mip2Size)*2);
    GTEST_ASSERT_EQ(texture->byteSize(0),mip0Size);
    GTEST_ASSERT_EQ(texture->byteSize(1),mip1Size);
    GTEST_ASSERT_EQ(texture->byteSize(2),mip2Size);
}

TEST(Texture, TextureSize1D)
{
    auto texture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R32G32B32A32_FLOAT,Texture::Type::TEXTURE_1D,Texture::UsageFlags::STORAGE,128,1,1,3,2));
    uint64_t mip0Size = Pixels::size(Pixels::Format::R32G32B32A32_FLOAT)*128;
    uint64_t mip1Size = Pixels::size(Pixels::Format::R32G32B32A32_FLOAT)*(128>>1);
    uint64_t mip2Size = Pixels::size(Pixels::Format::R32G32B32A32_FLOAT)*(128>>2);
    GTEST_ASSERT_EQ(texture->byteSize(),(mip0Size+mip1Size+mip2Size)*2);
    GTEST_ASSERT_EQ(texture->byteSize(0),mip0Size);
    GTEST_ASSERT_EQ(texture->byteSize(1),mip1Size);
    GTEST_ASSERT_EQ(texture->byteSize(2),mip2Size);
}

TEST(Texture, TextureSize3D)
{
    auto texture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R32G32B32A32_FLOAT,Texture::Type::TEXTURE_3D,Texture::UsageFlags::STORAGE,128,64,4,3,1));
    uint64_t mip0Size = Pixels::size(Pixels::Format::R32G32B32A32_FLOAT)*128*64*4;
    uint64_t mip1Size = Pixels::size(Pixels::Format::R32G32B32A32_FLOAT)*(128>>1)*(64>>1)*(4>>1);
    uint64_t mip2Size = Pixels::size(Pixels::Format::R32G32B32A32_FLOAT)*(128>>2)*(64>>2)*(4>>2);
    GTEST_ASSERT_EQ(texture->byteSize(),(mip0Size+mip1Size+mip2Size));
    GTEST_ASSERT_EQ(texture->byteSize(0),mip0Size);
    GTEST_ASSERT_EQ(texture->byteSize(1),mip1Size);
    GTEST_ASSERT_EQ(texture->byteSize(2),mip2Size);
}

TEST(Texture, TextureSizeCubeMap)
{
    auto texture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R32G32B32A32_FLOAT,Texture::Type::TEXTURE_CUBE,Texture::UsageFlags::STORAGE,128,128,1,3,6));
    uint64_t mip0Size = Pixels::size(Pixels::Format::R32G32B32A32_FLOAT)*128*128;
    uint64_t mip1Size = Pixels::size(Pixels::Format::R32G32B32A32_FLOAT)*(128>>1)*(128>>1);
    uint64_t mip2Size = Pixels::size(Pixels::Format::R32G32B32A32_FLOAT)*(128>>2)*(128>>2);
    GTEST_ASSERT_EQ(texture->byteSize(),(mip0Size+mip1Size+mip2Size)*6);
    GTEST_ASSERT_EQ(texture->byteSize(0),mip0Size);
    GTEST_ASSERT_EQ(texture->byteSize(1),mip1Size);
    GTEST_ASSERT_EQ(texture->byteSize(2),mip2Size);
}
#ifdef SLAG_DEBUG
TEST(Texture, ErrorWithDepth1D)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    EXPECT_DEATH([this]{std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R32G32B32A32_FLOAT,Texture::Type::TEXTURE_1D,Texture::UsageFlags::STORAGE,128,1,2,3,1));}(),"Non 3D textures must only have a depth of 1");
}

TEST(Texture, ErrorWithDepth2D)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    EXPECT_DEATH([this]{std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R32G32B32A32_FLOAT,Texture::Type::TEXTURE_2D,Texture::UsageFlags::STORAGE,128,128,2,3,1));}(),"Non 3D textures must only have a depth of 1");
}

TEST(Texture, ErrorWithDepthCube)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    EXPECT_DEATH([this]{std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R32G32B32A32_FLOAT,Texture::Type::TEXTURE_CUBE,Texture::UsageFlags::STORAGE,128,128,2,3,6));}(),"Non 3D textures must only have a depth of 1");
}

TEST(Texture, ErrorWithArray3D)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    EXPECT_DEATH([this]{std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R32G32B32A32_FLOAT,Texture::Type::TEXTURE_3D,Texture::UsageFlags::STORAGE,128,128,2,1,2));}(),"3D textures must only have one layer");
}

TEST(Texture, ErrorWithHeight1D)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    EXPECT_DEATH([this]{std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R32G32B32A32_FLOAT,Texture::Type::TEXTURE_1D,Texture::UsageFlags::STORAGE,128,128,1,1,1));}(),"1D textures must have a height of 1");
}
TEST(Texture, ErrorWithNoWidth)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    EXPECT_DEATH([this]{std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R32G32B32A32_FLOAT,Texture::Type::TEXTURE_2D,Texture::UsageFlags::STORAGE,0,1,1,1,1));}(),"Width, height, depth, mipLevels and layers must be at least 1");
}

TEST(Texture, ErrorWithNoHeight)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    EXPECT_DEATH([this]{std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R32G32B32A32_FLOAT,Texture::Type::TEXTURE_2D,Texture::UsageFlags::STORAGE,128,0,1,1,1));}(),"Width, height, depth, mipLevels and layers must be at least 1");
}
TEST(Texture, ErrorWithNoDepth)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    EXPECT_DEATH([this]{std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R32G32B32A32_FLOAT,Texture::Type::TEXTURE_3D,Texture::UsageFlags::STORAGE,128,128,0,1,1));}(),"Width, height, depth, mipLevels and layers must be at least 1");
}
TEST(Texture, ErrorWithNoMip)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    EXPECT_DEATH([this]{std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R32G32B32A32_FLOAT,Texture::Type::TEXTURE_3D,Texture::UsageFlags::STORAGE,128,128,2,0,1));}(),"Width, height, depth, mipLevels and layers must be at least 1");
}
TEST(Texture, ErrorWithNoLayer)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    EXPECT_DEATH([this]{std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R32G32B32A32_FLOAT,Texture::Type::TEXTURE_2D,Texture::UsageFlags::STORAGE,128,128,1,1,0));}(),"Width, height, depth, mipLevels and layers must be at least 1");

}
#endif

TEST(Texture, CreateWithData)
{
    struct Uint8Pixel
    {
        uint8_t r, g, b, a;
    };
    struct FloatPixel
    {
        float r, g, b, a;
    };
    std::vector<Uint8Pixel> uint8Data((64 * 64 + 32 * 32) * 2);
    std::vector<FloatPixel> floatData((64 * 64 + 32 * 32) * 2);
    size_t index = 0;
    for (; index < 64 * 64; index++)
    {
        uint8Data[index] = {255, 0, 0, 255};
        floatData[index] = {0.5f, 0, 0, 1.0f};
    }
    size_t start = 64 * 64;
    for (; index < start + 32 * 32; index++)
    {
        uint8Data[index] = {255, 120, 0, 255};
        floatData[index] = {0.5f, .3f, 0, 1.0f};
    }
    start += 32 * 32;
    for (; index < start + 64 * 64; index++)
    {
        uint8Data[index] = {0, 0, 255, 255};
        floatData[index] = {0, 0, 1.0f, 1.0f};
    }
    start += 64 * 64;
    for (; index < start + 32 * 32; index++)
    {
        uint8Data[index] = {0, 13, 255, 255};
        floatData[index] = {.7, 0, 1.0f, 1.0f};
    }

    TextureBufferMapping copyDataUint8[] =
    {
        {
            .bufferOffset = 0,
            .textureSubresource =
            {
                .aspectFlags = Pixels::AspectFlags::COLOR,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .textureOffset = {0,0,0},
            .textureExtent = {64,64,1}
        },
        {
            .bufferOffset = 64 * 64 * sizeof(Uint8Pixel),
            .textureSubresource =
            {
                .aspectFlags = Pixels::AspectFlags::COLOR,
                .mipLevel = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .textureOffset = {0,0,0},
            .textureExtent = {32,32,1}
        },
        {
            .bufferOffset = (64 * 64 + 32 * 32) * sizeof(Uint8Pixel),
            .textureSubresource =
            {
                .aspectFlags = Pixels::AspectFlags::COLOR,
                .mipLevel = 0,
                .baseArrayLayer = 1,
                .layerCount = 1
            },
            .textureOffset = {0,0,0},
            .textureExtent = {64,64,1}
        },
        {
            .bufferOffset = ((64 * 64) * 2 + 32 * 32) * sizeof(Uint8Pixel),
            .textureSubresource =
            {
                .aspectFlags = Pixels::AspectFlags::COLOR,
                .mipLevel = 1,
                .baseArrayLayer = 1,
                .layerCount = 1
            },
            .textureOffset = {0,0,0},
            .textureExtent = {32,32,1}
        },
    };

    TextureBufferMapping copyDataFloat[] =
    {
        {
            .bufferOffset = 0,
            .textureSubresource =
            {
                .aspectFlags = Pixels::AspectFlags::COLOR,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .textureOffset = {0,0,0},
            .textureExtent = {64,64,1}
        },
        {
            .bufferOffset = 64 * 64 * sizeof(FloatPixel),
            .textureSubresource =
            {
                .aspectFlags = Pixels::AspectFlags::COLOR,
                .mipLevel = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .textureOffset = {0,0,0},
            .textureExtent = {32,32,1}
        },
        {
            .bufferOffset = (64 * 64 + 32 * 32) * sizeof(FloatPixel),
            .textureSubresource =
            {
                .aspectFlags = Pixels::AspectFlags::COLOR,
                .mipLevel = 0,
                .baseArrayLayer = 1,
                .layerCount = 1
            },
            .textureOffset = {0,0,0},
            .textureExtent = {64,64,1}
        },
        {
            .bufferOffset = ((64 * 64) * 2 + 32 * 32) * sizeof(FloatPixel),
            .textureSubresource =
            {
                .aspectFlags = Pixels::AspectFlags::COLOR,
                .mipLevel = 1,
                .baseArrayLayer = 1,
                .layerCount = 1
            },
            .textureOffset = {0,0,0},
            .textureExtent = {32,32,1}
        },
    };


    auto uintTex = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UINT,
                                                                Texture::Type::TEXTURE_2D,
                                                                Texture::UsageFlags::SAMPLED_IMAGE, 64, 64, 1, 2, 2,
                                                                Texture::SampleCount::ONE, uint8Data.data(), uint8Data.size()*sizeof(Uint8Pixel), copyDataUint8, 4));
    auto floatTex = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R32G32B32A32_FLOAT,
                                                                 Texture::Type::TEXTURE_2D,
                                                                 Texture::UsageFlags::SAMPLED_IMAGE, 64, 64, 1, 2, 2,
                                                                 Texture::SampleCount::ONE, floatData.data(), floatData.size()*sizeof(FloatPixel), copyDataFloat, 4));

    auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::TRANSFER));
    auto finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));

    auto uintDownload = std::unique_ptr<Buffer>(
        Buffer::newBuffer(uintTex->byteSize(), Buffer::Accessibility::CPU_AND_GPU, Buffer::UsageFlags::DATA_BUFFER));
    auto floatDownload = std::unique_ptr<Buffer>(
        Buffer::newBuffer(floatTex->byteSize(), Buffer::Accessibility::CPU_AND_GPU, Buffer::UsageFlags::DATA_BUFFER));


    commandBuffer->begin();


    commandBuffer->copyTextureToBuffer(uintTex.get(), uintDownload.get(), copyDataUint8, 4);
    commandBuffer->copyTextureToBuffer(floatTex.get(), floatDownload.get(), copyDataFloat, 4);

    commandBuffer->end();

    auto cmdPtr = commandBuffer.get();

    SemaphoreValue signal{.semaphore = finished.get(), .value = 1};

    QueueSubmissionBatch batch
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = &cmdPtr,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1,
    };
    slagGraphicsCard()->transferQueue()->submit(&batch, 1);
    finished->waitForValue(1);

    Uint8Pixel* uintPtr = uintDownload->as<Uint8Pixel>();
    FloatPixel* floatPtr = floatDownload->as<FloatPixel>();
    for (size_t i = 0; i < uint8Data.size(); i++)
    {
        auto uint8DownloadPixel = uintPtr[i];
        auto floatDownloadPixel = floatPtr[i];
        auto uint8OriginalPixel = uint8Data[i];
        auto floatOriginalPixel = floatData[i];

        GTEST_ASSERT_EQ(uint8DownloadPixel.r, uint8OriginalPixel.r);
        GTEST_ASSERT_EQ(uint8DownloadPixel.g, uint8OriginalPixel.g);
        GTEST_ASSERT_EQ(uint8DownloadPixel.b, uint8OriginalPixel.b);
        GTEST_ASSERT_EQ(uint8DownloadPixel.a, uint8OriginalPixel.a);

        GTEST_ASSERT_EQ(floatDownloadPixel.r, floatOriginalPixel.r);
        GTEST_ASSERT_EQ(floatDownloadPixel.g, floatOriginalPixel.g);
        GTEST_ASSERT_EQ(floatDownloadPixel.b, floatOriginalPixel.b);
        GTEST_ASSERT_EQ(floatDownloadPixel.a, floatOriginalPixel.a);
    }
}

#define DEFINITION(SlagName, DxName, VulkanName, VkImageAspectFlags, VkComponentSwizzle_r, VkComponentSwizzle_g, VkComponentSwizzle_b, VkComponentSwizzle_a, totalBits,colorBits,depthBits,stencilBits, Aspects)\
    TEST(Texture, SlagName)\
    {\
        auto properties = Pixels::formatProperties(Pixels::Format::SlagName);\
        if(properties.tiling == PixelFormatProperties::Tiling::UNSUPPORTED)\
        {\
            GTEST_SKIP();\
        }\
        auto texture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::SlagName,Texture::Type::TEXTURE_2D,properties.validUsageFlags,64,64,1,1,1));\
    }
SLAG_TEXTURE_FORMAT_DEFINTITIONS(DEFINITION)
#undef DEFINITION

#ifdef SLAG_DEBUG
TEST(Texture, CopyMultiAspect)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::GRAPHICS));
    auto finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    auto depthTexture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::D24_UNORM_S8_UINT,Texture::Type::TEXTURE_2D,Texture::UsageFlags::DEPTH_STENCIL_ATTACHMENT,32,32,1,1,1));
    auto result = std::unique_ptr<Buffer>(Buffer::newBuffer(depthTexture->byteSize(),Buffer::Accessibility::CPU_AND_GPU, Buffer::UsageFlags::DATA_BUFFER));


    commandBuffer->begin();
    commandBuffer->clearTexture(depthTexture.get(),ClearDepthStencilValue{.depth = .5f,.stencil = 1});
    commandBuffer->insertBarrier(
        TextureBarrier
        {
            depthTexture.get(),
            0,
            1,
            0,
            1,
            BarrierAccessFlags::CLEAR,
            BarrierAccessFlags::TRANSFER_READ,
            PipelineStageFlags::CLEAR_DEPTH,
            PipelineStageFlags::TRANSFER
        });
    TextureBufferMapping copyData
    {
        .bufferOffset = 0,
        .textureSubresource =
     {
            .aspectFlags = Pixels::AspectFlags::DEPTH_STENCIL,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .textureOffset = {0,0,0},
        .textureExtent = {.width = depthTexture->width(),.height = depthTexture->height(),.depth = 1},
    };
    EXPECT_DEATH(commandBuffer->copyTextureToBuffer(depthTexture.get(),result.get(),&copyData,1),"Only a single aspect may be specified per subresource");
}
#endif

TEST(Texture, CopyPartialAspects)
{
    auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::GRAPHICS));
    auto finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    auto depthTexture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::D24_UNORM_S8_UINT,Texture::Type::TEXTURE_2D,Texture::UsageFlags::DEPTH_STENCIL_ATTACHMENT,32,32,1,1,1));
    auto result = std::unique_ptr<Buffer>(Buffer::newBuffer(depthTexture->byteSize(),Buffer::Accessibility::CPU_AND_GPU, Buffer::UsageFlags::DATA_BUFFER));


    commandBuffer->begin();
    commandBuffer->clearTexture(depthTexture.get(),ClearDepthStencilValue{.depth = 0.0f,.stencil = 1});
    commandBuffer->insertBarrier(
        TextureBarrier
        {
            depthTexture.get(),
            0,
            1,
            0,
            1,
            BarrierAccessFlags::CLEAR,
            BarrierAccessFlags::TRANSFER_READ,
            PipelineStageFlags::CLEAR_DEPTH,
            PipelineStageFlags::TRANSFER
        });
    TextureBufferMapping copyData[]
    {
    {
            .bufferOffset = 0,
            .textureSubresource =
         {
                .aspectFlags = Pixels::AspectFlags::DEPTH,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .textureOffset = {0,0,0},
            .textureExtent = {.width = depthTexture->width(),.height = depthTexture->height(),.depth = 1},
        },
        {
            .bufferOffset = Pixels::size(depthTexture->format(),Pixels::AspectFlags::DEPTH)*depthTexture->width()*depthTexture->height(),
            .textureSubresource =
         {
                .aspectFlags = Pixels::AspectFlags::STENCIL,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .textureOffset = {0,0,0},
            .textureExtent = {.width = depthTexture->width(),.height = depthTexture->height(),.depth = 1},
        }
    };
    commandBuffer->copyTextureToBuffer(depthTexture.get(),result.get(),copyData,2);

    commandBuffer->end();

    auto bufferPtr = commandBuffer.get();

    SemaphoreValue signal
    {
        .semaphore = finished.get(),
        .value = 1
    };
    QueueSubmissionBatch batch
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = &bufferPtr,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1,
    };
    slagGraphicsCard()->graphicsQueue()->submit(&batch,1);
    finished->waitForValue(1);

    auto byteData = result->as<unsigned char>();
    auto i=0;
    for (; i<Pixels::size(depthTexture->format(),Pixels::AspectFlags::DEPTH)*depthTexture->width()*depthTexture->height(); i+=4)
    {
        int x = byteData[i+2];
        x = (x << 8) | byteData[i+1];
        x = (x << 8) | byteData[i];

        if (x & 0x800000)
        {
            x |= ~0xffffff;
        }

        const float Q = 1.0 / (0x7fffff);
        float floatRep = x*Q;
        if (floatRep != 0.0f)
        {
            std::cout << i << std::endl;
        }
        GTEST_ASSERT_EQ(floatRep, 0.0f);
    }
    for (;i<result->countAsArray<uint8_t>();i++)
    {
        GTEST_ASSERT_EQ(byteData[i], 1);
    }
}
