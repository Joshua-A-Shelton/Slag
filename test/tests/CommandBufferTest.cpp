#include "gtest/gtest.h"
#include "slag/SlagLib.h"
using namespace slag;

TEST(CommandBuffer, StartFinished)
{
    std::unique_ptr<CommandBuffer> buffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GpuQueue::Graphics));
    ASSERT_TRUE(buffer->isFinished());
}

TEST(CommandBuffer, InsertBarriers)
{
    std::unique_ptr<CommandBuffer> buffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GpuQueue::Graphics));
    std::unique_ptr<Texture> img = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UINT,slag::Texture::TEXTURE_2D,100,100,1,1,1,TextureUsageFlags::SAMPLED_IMAGE | TextureUsageFlags::RENDER_TARGET_ATTACHMENT | TextureUsageFlags::STORAGE));
    ImageBarrier imageBarrier
    {
        .texture = img.get(),
    };
    buffer->begin();
    std::vector<Texture::Layout> layouts =
    {
#define DEFINITION(slagName, vulkanName, directXName,directXResourceName) Texture::slagName,
                    TEXTURE_LAYOUT_DEFINTITIONS(DEFINITION)
#undef DEFINITION
    };
    imageBarrier.oldLayout = Texture::UNDEFINED;
    for(size_t i=0; i< layouts.size(); i++)
    {
        auto newLayout = layouts[i];
        if(newLayout != Texture::UNDEFINED && newLayout != Texture::DEPTH_READ && newLayout != Texture::DEPTH_WRITE)
        {
            imageBarrier.newLayout = newLayout;
            break;
        }
        imageBarrier.newLayout = newLayout;
        buffer->insertBarriers(&imageBarrier,1, nullptr,0, nullptr,0);
        imageBarrier.oldLayout = newLayout;

    }

    std::unique_ptr<Texture> depthImg = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::D32_FLOAT_S8X24_UINT,slag::Texture::TEXTURE_2D,100,100,1,1,1,TextureUsageFlags::DEPTH_STENCIL_ATTACHMENT));
    imageBarrier.texture = depthImg.get();
    imageBarrier.oldLayout = Texture::UNDEFINED;
    imageBarrier.newLayout = Texture::GENERAL;
    buffer->insertBarriers(&imageBarrier,1, nullptr,0, nullptr,0);
    imageBarrier.oldLayout = Texture::GENERAL;
    imageBarrier.newLayout = Texture::TRANSFER_SOURCE;
    buffer->insertBarriers(&imageBarrier,1, nullptr,0, nullptr,0);
    imageBarrier.oldLayout = Texture::TRANSFER_SOURCE;
    imageBarrier.newLayout = Texture::TRANSFER_DESTINATION;
    buffer->insertBarriers(&imageBarrier,1, nullptr,0, nullptr,0);
    imageBarrier.oldLayout = Texture::TRANSFER_DESTINATION;
    imageBarrier.newLayout = Texture::DEPTH_READ;
    buffer->insertBarriers(&imageBarrier,1, nullptr,0, nullptr,0);
    imageBarrier.oldLayout = Texture::DEPTH_READ;
    imageBarrier.newLayout = Texture::DEPTH_WRITE;
    buffer->insertBarriers(&imageBarrier,1, nullptr,0, nullptr,0);


    buffer->end();
    SlagLib::graphicsCard()->graphicsQueue()->submit(buffer.get());
    buffer->waitUntilFinished();
    GTEST_FAIL();

}

TEST(CommandBuffer, ClearColorImage)
{
    GTEST_SKIP();
}

TEST(CommandBuffer, UpdateMipChain)
{
    GTEST_SKIP();
}

TEST(CommandBuffer, CopyBuffer)
{
    GTEST_SKIP();
}

TEST(CommandBuffer, CopyImageToBuffer)
{
    GTEST_SKIP();
}

TEST(CommandBuffer, CopyBufferToImage)
{
    GTEST_SKIP();
}

TEST(CommandBuffer, Blit)
{
    GTEST_SKIP();
}