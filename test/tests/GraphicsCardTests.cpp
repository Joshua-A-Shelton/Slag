#include <stb_image.h>

#include "gtest/gtest.h"
#include "slag/SlagLib.h"
using namespace slag;
TEST(GraphicsCard, DefragBuffers)
{
    std::vector<std::unique_ptr<Buffer>> buffers(3);
    std::vector<unsigned char> rawData(256,128);
    std::vector<unsigned char> targetData(256,255);
    for(uint32_t i = 0; i < buffers.size()-1; ++i)
    {
        buffers[i] = std::unique_ptr<Buffer>(Buffer::newBuffer(rawData.data(), rawData.size(),slag::Buffer::CPU_AND_GPU,Buffer::DATA_BUFFER));
    }
    buffers[buffers.size()-1] = std::unique_ptr<Buffer>(Buffer::newBuffer(targetData.data(), targetData.size(),slag::Buffer::CPU_AND_GPU,Buffer::DATA_BUFFER));
    for(int i = 0; i < buffers.size()-1; ++i)
    {
        buffers[i] = std::unique_ptr<Buffer>(nullptr);
    }
    auto& targetBuffer = buffers[buffers.size()-1];
    auto memLocation = targetBuffer->cpuHandle();
    SlagLib::graphicsCard()->defragmentMemory();
    auto memLocation2 = targetBuffer->cpuHandle();

    GTEST_ASSERT_NE(memLocation, memLocation2);
    auto targetDataMoved = targetBuffer->downloadData();
    GTEST_ASSERT_EQ(targetData.size(), targetDataMoved.size());
    for (auto i=0; i< targetData.size(); i++)
    {
        GTEST_ASSERT_EQ(targetData[i], (unsigned char)targetDataMoved[i]);
    }

}

TEST(GraphicsCard, DefragTextures)
{
    std::vector<std::unique_ptr<Texture>> textures(3);
    for (auto i = 0; i < textures.size()-1; ++i)
    {
        textures[i] = std::unique_ptr<Texture>(Texture::newTexture("resources/solid-color.png",Pixels::Format::R8G8B8A8_UNORM,2,TextureUsageFlags::SAMPLED_IMAGE,Texture::SHADER_RESOURCE));
    }
    textures[textures.size()-1] = std::unique_ptr<Texture>(Texture::newTexture("resources/test-img.png",Pixels::Format::R8G8B8A8_UNORM,3,TextureUsageFlags::SAMPLED_IMAGE,Texture::TRANSFER_SOURCE));
    for (auto i = 0; i < textures.size()-1; ++i)
    {
        textures[i] = std::unique_ptr<Texture>(nullptr);
    }
    auto& texture = textures[textures.size()-1];

    SlagLib::graphicsCard()->defragmentMemory();

    auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GpuQueue::GRAPHICS));
    auto dataBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(texture->width()*texture->height()*Pixels::pixelBytes(texture->format()),Buffer::CPU_AND_GPU,Buffer::DATA_BUFFER));
    commandBuffer->begin();
    commandBuffer->copyImageToBuffer(texture.get(),Texture::TRANSFER_SOURCE,0,1,0,dataBuffer.get(),0);
    commandBuffer->end();
    SlagLib::graphicsCard()->graphicsQueue()->submit(commandBuffer.get());
    commandBuffer->waitUntilFinished();

    auto data = dataBuffer->downloadData();
    auto file = std::filesystem::absolute("resources/test-img.png");
    int w,h,channels;
    auto rawBytes = stbi_load(file.string().c_str(),&w,&h,&channels,4);
    std::vector<std::byte> groundTruth(w*h*channels);
    memcpy(groundTruth.data(),rawBytes,w*h*channels);
    stbi_image_free(rawBytes);

    for(size_t i=0; i< w*h*channels; i++)
    {
        GTEST_ASSERT_EQ(data[i],groundTruth[i]);
    }

}