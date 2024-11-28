#include "gtest/gtest.h"
#include "slag/SlagLib.h"
#include "stb_image.h"
#include "filesystem"
#include <lodepng.h>

using namespace slag;

struct RawImageDeleter
{
    void operator()(unsigned char* data)
    {

    }
};

TEST(Texture, LoadFromFile)
{
    std::unique_ptr<Texture> texture = std::unique_ptr<Texture>(Texture::newTexture("resources/Crucible.png",Pixels::R8G8B8A8_UINT,1,TextureUsageFlags::SAMPLED_IMAGE,Texture::Layout::TRANSFER_SOURCE));
    GTEST_ASSERT_GE(texture->sampleCount() ,1);
    GTEST_ASSERT_GE(texture->mipLevels() ,1);
    GTEST_ASSERT_GE(texture->type() ,Texture::Type::TEXTURE_2D);
    GTEST_ASSERT_GE(texture->layers() ,1);
    GTEST_ASSERT_GE(texture->width() ,1920);
    GTEST_ASSERT_GE(texture->height() ,1080);
    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GpuQueue::TRANSFER));
    std::unique_ptr<Buffer> dataBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(1920*1080*sizeof(unsigned char)*4,Buffer::CPU_AND_GPU,Buffer::DATA_BUFFER));
    commandBuffer->begin();
    commandBuffer->copyImageToBuffer(texture.get(),Texture::Layout::TRANSFER_SOURCE,0,1,0,dataBuffer.get(),0);
    commandBuffer->end();
    SlagLib::graphicsCard()->transferQueue()->submit(commandBuffer.get());
    commandBuffer->waitUntilFinished();
    auto data = dataBuffer->downloadData();

    int w, h, channels;

    auto rawBytes = stbi_load(std::filesystem::absolute("resources/Crucible.png").string().c_str(),&w,&h,&channels,4);
    std::vector<std::byte> groundTruth(w*h*channels);
    memcpy(groundTruth.data(),rawBytes,w*h*channels);
    stbi_image_free(rawBytes);
    for(size_t i=0; i< w*h*channels; i++)
    {
        GTEST_ASSERT_TRUE(data[i] == groundTruth[i]);
    }

}

TEST(Texture, MultiSampled)
{

    std::unique_ptr<Texture> texture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R32G32B32A32_FLOAT,slag::Texture::TEXTURE_2D,500,700,1,1,8,TextureUsageFlags::RENDER_TARGET_ATTACHMENT));
    GTEST_ASSERT_GE(texture->sampleCount() ,8);
    GTEST_ASSERT_GE(texture->mipLevels() ,1);
    GTEST_ASSERT_GE(texture->type() ,Texture::Type::TEXTURE_2D);
    GTEST_ASSERT_GE(texture->layers() ,1);
    GTEST_ASSERT_GE(texture->width() ,500);
    GTEST_ASSERT_GE(texture->height() ,700);
}

TEST(Texture, MipMapped)
{
    std::unique_ptr<Texture> texture = std::unique_ptr<Texture>(Texture::newTexture("resources/test-img.png",Pixels::R8G8B8A8_UINT,5,TextureUsageFlags::SAMPLED_IMAGE,Texture::Layout::TRANSFER_SOURCE));

    GTEST_ASSERT_GE(texture->sampleCount() ,1);
    GTEST_ASSERT_GE(texture->mipLevels() ,5);
    GTEST_ASSERT_GE(texture->type() ,Texture::Type::TEXTURE_2D);
    GTEST_ASSERT_GE(texture->layers() ,1);
    GTEST_ASSERT_GE(texture->width() ,100);
    GTEST_ASSERT_GE(texture->height() ,100);

    std::unique_ptr<Texture> flatMipped = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UINT,slag::Texture::TEXTURE_2D,150,100,1,1,1,TextureUsageFlags::SAMPLED_IMAGE));
    std::unique_ptr<Buffer> dataBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(flatMipped->width()*flatMipped->height()*sizeof(unsigned char)*4,Buffer::CPU_AND_GPU,Buffer::DATA_BUFFER));

    auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GpuQueue::GRAPHICS));
    commandBuffer->begin();
    commandBuffer->updateMipChain(texture.get(),0,Texture::Layout::TRANSFER_SOURCE,Texture::Layout::TRANSFER_SOURCE,Texture::Layout::TRANSFER_SOURCE,Texture::Layout::TRANSFER_SOURCE,PipelineStageFlags::TRANSFER,PipelineStageFlags::ALL_GRAPHICS);
    commandBuffer->clearColorImage(flatMipped.get(),ClearColor{0,0,0,0},slag::Texture::UNDEFINED,slag::Texture::TRANSFER_DESTINATION,PipelineStageFlags::NONE,PipelineStageFlags::ALL_GRAPHICS);
    Rectangle srcArea{.offset{},.extent{100,100}};
    Rectangle dstArea{.offset{},.extent{100,100}};
    commandBuffer->blit(texture.get(),Texture::TRANSFER_SOURCE,0,0,srcArea,flatMipped.get(),Texture::TRANSFER_DESTINATION,0,0,dstArea,Sampler::Filter::NEAREST);
    dstArea.offset.x = 100;
    for(uint32_t i=1; i< texture->mipLevels(); i++)
    {
        srcArea.extent.width = srcArea.extent.width>>1;
        srcArea.extent.height = srcArea.extent.height>>1;
        dstArea.extent.width = dstArea.extent.width>>1;
        dstArea.extent.height = dstArea.extent.height>>1;

        commandBuffer->blit(texture.get(),Texture::TRANSFER_SOURCE,0,i,srcArea,flatMipped.get(),Texture::TRANSFER_DESTINATION,0,0,dstArea,Sampler::Filter::NEAREST);

        dstArea.offset.y += dstArea.extent.height;
    }
    ImageBarrier flatMippedBarrier
            {
                    .texture = flatMipped.get(),
                    .oldLayout = Texture::TRANSFER_DESTINATION,
                    .newLayout = Texture::TRANSFER_SOURCE,
                    .accessBefore = BarrierAccessFlags::ALL_WRITE,
                    .accessAfter = BarrierAccessFlags::ALL_READ | BarrierAccessFlags::ALL_WRITE,
                    .syncBefore = PipelineStageFlags::TRANSFER,
                    .syncAfter = PipelineStageFlags::ALL_COMMANDS
            };
    commandBuffer->insertBarriers(&flatMippedBarrier,1, nullptr,0, nullptr,0);

    commandBuffer->copyImageToBuffer(flatMipped.get(),Texture::Layout::TRANSFER_SOURCE,0,1,0,dataBuffer.get(),0);
    commandBuffer->end();
    SlagLib::graphicsCard()->graphicsQueue()->submit(commandBuffer.get());
    commandBuffer->waitUntilFinished();

    int w, h, channels;
    auto file = std::filesystem::absolute("resources/test-img-mipped.png");
    auto rawBytes = stbi_load(file.string().c_str(),&w,&h,&channels,4);
    std::vector<std::byte> groundTruth(w*h*channels);
    memcpy(groundTruth.data(),rawBytes,w*h*channels);
    stbi_image_free(rawBytes);

    auto data = dataBuffer->downloadData();

    for(size_t i=0; i< w*h*channels; i++)
    {
        GTEST_ASSERT_EQ(data[i],groundTruth[i]);
    }

}