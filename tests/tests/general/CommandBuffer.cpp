#ifndef SLAG_DISCREET_TEXTURE_LAYOUTS
#include <gtest/gtest.h>
#include <slag/Slag.h>
#include <memory>
using namespace slag;
TEST(CommandBuffer, ClearColor)
{
    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::GRAPHICS));
    std::unique_ptr<Semaphore> finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore());
    std::unique_ptr<Texture> renderTarget = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::RENDER_TARGET_ATTACHMENT,32,32,1,1));
    std::unique_ptr<Texture> sampled = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::SAMPLED_IMAGE,32,32,1,4));
    std::unique_ptr<Texture> input = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::INPUT_ATTACHMENT,32,32,3,1));
    std::unique_ptr<Buffer> rtBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(renderTarget->byteSize(),Buffer::Accessibility::CPU_AND_GPU));
    std::unique_ptr<Buffer> sampledBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(sampled->byteSize(),Buffer::Accessibility::CPU_AND_GPU));
    std::unique_ptr<Buffer> inputBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(input->byteSize(),Buffer::Accessibility::CPU_AND_GPU));

    commandBuffer->begin();

    commandBuffer->clearTexture(renderTarget.get(),ClearColor{.floats = {1.0f,0.0f,0.0f,1.0f}});
    commandBuffer->clearTexture(sampled.get(),ClearColor{.floats = {0.0f,1.0f,0.0f,1.0f}});
    commandBuffer->clearTexture(input.get(),ClearColor{.floats = {0.0f,0.0f,1.0f,1.0f}});


    TextureBarrier barriers[3]
    {
        {
                .texture = renderTarget.get(),
                .accessBefore = BarrierAccessFlags::CLEAR,
                .accessAfter = BarrierAccessFlags::TRANSFER_READ,
                .syncBefore = PipelineStageFlags::ALL_COMMANDS,
                .syncAfter = PipelineStageFlags::TRANSFER,
            },
        {
                .texture = sampled.get(),
                .accessBefore = BarrierAccessFlags::CLEAR,
                .accessAfter = BarrierAccessFlags::TRANSFER_READ,
                .syncBefore = PipelineStageFlags::ALL_COMMANDS,
                .syncAfter = PipelineStageFlags::TRANSFER,
        },
     {
            .texture = input.get(),
            .accessBefore = BarrierAccessFlags::CLEAR,
            .accessAfter = BarrierAccessFlags::TRANSFER_READ,
            .syncBefore = PipelineStageFlags::ALL_COMMANDS,
            .syncAfter = PipelineStageFlags::TRANSFER,
        }
    };
    commandBuffer->insertBarriers(barriers,3,nullptr,0,nullptr,0);
    TextureToBufferCopyData rtCopyData
   {
       .bufferOffset = 0,
       .subresource =
        {
            .aspectFlags = Pixels::AspectFlags::COLOR,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
   };
    commandBuffer->copyTextureToBuffer(renderTarget.get(),&rtCopyData,1,rtBuffer.get());

    TextureToBufferCopyData sampledCopyData[4]
    {
        {
            .bufferOffset = 0,
            .subresource =
            {
                .aspectFlags = Pixels::AspectFlags::COLOR,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        },
        {
            .bufferOffset = sampled->byteSize(0),
            .subresource =
            {
                .aspectFlags = Pixels::AspectFlags::COLOR,
                .mipLevel = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        },
        {
            .bufferOffset = sampled->byteSize(0)+sampled->byteSize(1),
            .subresource =
        {
                .aspectFlags = Pixels::AspectFlags::COLOR,
                .mipLevel = 2,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        },
        {
            .bufferOffset = sampled->byteSize(0)+sampled->byteSize(1)+sampled->byteSize(2),
            .subresource =
        {
                .aspectFlags = Pixels::AspectFlags::COLOR,
                .mipLevel = 3,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        }
    };
    commandBuffer->copyTextureToBuffer(sampled.get(),sampledCopyData,4,sampledBuffer.get());
    TextureToBufferCopyData inputCopyData[3]
    {
        {
            .bufferOffset = 0,
            .subresource =
            {
                .aspectFlags = Pixels::AspectFlags::COLOR,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        },
        {
            .bufferOffset = sampled->byteSize(0),
            .subresource =
            {
                .aspectFlags = Pixels::AspectFlags::COLOR,
                .mipLevel = 0,
                .baseArrayLayer = 1,
                .layerCount = 1
            }
        },
        {
            .bufferOffset = sampled->byteSize(0)*2,
            .subresource =
        {
                .aspectFlags = Pixels::AspectFlags::COLOR,
                .mipLevel = 0,
                .baseArrayLayer = 2,
                .layerCount = 1
            }
        }
    };
    commandBuffer->copyTextureToBuffer(input.get(),inputCopyData,3,inputBuffer.get());

    commandBuffer->end();

    CommandBuffer* submitBuffers[1] = {commandBuffer.get()};
    SemaphoreValue signal{.semaphore = finished.get(), .value = 1};
    slag::slagGraphicsCard()->graphicsQueue()->submit(submitBuffers,1,nullptr,0,&signal,1);
    finished->waitForValue(1);

    auto rtPtr = rtBuffer->as<unsigned char>();
    for (auto i=0; i< rtBuffer->size(); i+=4)
    {
        unsigned char red = rtPtr[i];
        unsigned char green = rtPtr[i+1];
        unsigned char blue = rtPtr[i+2];
        unsigned char alpha = rtPtr[i+3];
        GTEST_ASSERT_EQ(red,255);
        GTEST_ASSERT_EQ(green,0);
        GTEST_ASSERT_EQ(blue,0);
        GTEST_ASSERT_EQ(alpha,255);
    }
    auto sampledPtr = sampledBuffer->as<unsigned char>();
    for (auto i=0; i< sampledBuffer->size(); i+=4)
    {
        unsigned char red = sampledPtr[i];
        unsigned char green = sampledPtr[i+1];
        unsigned char blue = sampledPtr[i+2];
        unsigned char alpha = sampledPtr[i+3];
        GTEST_ASSERT_EQ(red,0);
        GTEST_ASSERT_EQ(green,255);
        GTEST_ASSERT_EQ(blue,0);
        GTEST_ASSERT_EQ(alpha,255);
    }
    auto inputPtr = inputBuffer->as<unsigned char>();
    for (auto i=0; i< inputBuffer->size(); i+=4)
    {
        unsigned char red = inputPtr[i];
        unsigned char green = inputPtr[i+1];
        unsigned char blue = inputPtr[i+2];
        unsigned char alpha = inputPtr[i+3];
        GTEST_ASSERT_EQ(red,0);
        GTEST_ASSERT_EQ(green,0);
        GTEST_ASSERT_EQ(blue,255);
        GTEST_ASSERT_EQ(alpha,255);
    }
}
#ifdef SLAG_DEBUG
TEST(CommandBuffer, ClearColorFailInRenderPass)
{
    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::GRAPHICS));
    std::unique_ptr<Semaphore> finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    std::unique_ptr<Texture> texture1 = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::RENDER_TARGET_ATTACHMENT,32,32,1,1));
    std::unique_ptr<Texture> texture2 = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::RENDER_TARGET_ATTACHMENT,32,32,1,1));
    commandBuffer->begin();

    Attachment colorAttachment
    {
        .texture = texture1.get(),
        .autoClear = false,
    };
    commandBuffer->beginRendering(&colorAttachment,1,nullptr,slag::Rectangle{.offset = {0,0},.extent = {texture1->width(),texture1->height()}});
    EXPECT_DEATH(commandBuffer->clearTexture(texture1.get(),ClearColor{.floats = {1.0f,0.0f,0.0f,1.0f}}),"Cleared texture in renderpass");
    EXPECT_DEATH(commandBuffer->clearTexture(texture2.get(),ClearColor{.floats = {1.0f,0.0f,0.0f,1.0f}}),"Cleared texture in renderpass");
}
#endif

TEST(CommandBuffer, ClearDepth)
{
    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::GRAPHICS));
    std::unique_ptr<Semaphore> finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    std::unique_ptr<Texture> depthTexture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::D32_FLOAT,Texture::Type::TEXTURE_2D,Texture::UsageFlags::DEPTH_STENCIL_ATTACHMENT,32,32,1,1));
    std::unique_ptr<Buffer> depthBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(depthTexture->byteSize(),Buffer::Accessibility::CPU_AND_GPU));


    commandBuffer->begin();
    commandBuffer->clearTexture(depthTexture.get(),ClearDepthStencilValue{.depth = 1,.stencil = 0});
    commandBuffer->insertBarrier(
        TextureBarrier
        {
            .texture = depthTexture.get(),
            .accessBefore = BarrierAccessFlags::CLEAR,
            .accessAfter = BarrierAccessFlags::TRANSFER_READ,
            .syncBefore = PipelineStageFlags::ALL_COMMANDS,
            .syncAfter = PipelineStageFlags::TRANSFER
        });
    TextureToBufferCopyData copyData
    {
        .bufferOffset = 0,
        .subresource =
     {
            .aspectFlags = Pixels::AspectFlags::DEPTH,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }
    };
    commandBuffer->copyTextureToBuffer(depthTexture.get(),&copyData,1,depthBuffer.get());
    commandBuffer->end();

    CommandBuffer* submitBuffers[1] = {commandBuffer.get()};
    SemaphoreValue signal{.semaphore = finished.get(), .value = 1};
    slag::slagGraphicsCard()->graphicsQueue()->submit(submitBuffers,1,nullptr,0,&signal,1);
    finished->waitForValue(1);

    auto depthPtr = depthBuffer->as<float>();
    for (auto i=0; i< depthBuffer->countAsArray<float>(); i++)
    {
        GTEST_ASSERT_EQ(depthPtr[i],1.0f);
    }
}

#ifdef SLAG_DEBUG
TEST(CommandBuffer, ClearDepthFailInRenderPass)
{
    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::GRAPHICS));
    std::unique_ptr<Semaphore> finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    std::unique_ptr<Texture> color = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::RENDER_TARGET_ATTACHMENT,32,32,1,1));
    std::unique_ptr<Texture> texture1 = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::DEPTH_STENCIL_ATTACHMENT,32,32,1,1));
    std::unique_ptr<Texture> texture2 = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::DEPTH_STENCIL_ATTACHMENT,32,32,1,1));
    commandBuffer->begin();

    Attachment colorAttachment
    {
        .texture = color.get(),
        .autoClear = false,
    };
    Attachment depthAttachment
    {
        .texture = texture1.get(),
        .autoClear = false,
    };
    commandBuffer->beginRendering(&colorAttachment,1,&depthAttachment,slag::Rectangle{.offset = {0,0},.extent = {texture1->width(),texture1->height()}});
    ASSERT_DEATH(commandBuffer->clearTexture(texture1.get(),ClearDepthStencilValue{.depth=1.0f,.stencil=0}),"Cleared texture in renderpass");
    ASSERT_DEATH(commandBuffer->clearTexture(texture2.get(),ClearDepthStencilValue{.depth=1.0f,.stencil=0}),"Cleared texture in renderpass");
}
#endif

TEST(CommandBuffer, UpdateMip)
{
    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::GRAPHICS));
    std::unique_ptr<Semaphore> finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    struct byteColor
    {
        unsigned char r;
        unsigned char g;
        unsigned char b;
        unsigned char a;
    };
    std::vector<byteColor> texels(32*32,byteColor{255,127,50,25});
    std::unique_ptr<Texture> texture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::SAMPLED_IMAGE,32,32,1,2,texels.data(),1,1));
    std::unique_ptr<Buffer> textureBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(texture->byteSize(1),Buffer::Accessibility::CPU_AND_GPU));
    commandBuffer->begin();

    commandBuffer->updateMip(texture.get(),0,0,1);
    commandBuffer->insertBarrier(
        TextureBarrier
        {
            .texture = texture.get(),
            .accessBefore = BarrierAccessFlags::BLIT_READ | BarrierAccessFlags::BLIT_WRITE,
            .accessAfter = BarrierAccessFlags::TRANSFER_READ,
            .syncBefore = PipelineStageFlags::BLIT,
            .syncAfter = PipelineStageFlags::TRANSFER
        });
    TextureToBufferCopyData copyData
    {
        .bufferOffset = 0,
        .subresource =
        {
            .aspectFlags = Pixels::AspectFlags::COLOR,
            .mipLevel = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }
    };
    commandBuffer->copyTextureToBuffer(texture.get(),&copyData,1,textureBuffer.get());

    commandBuffer->end();

    CommandBuffer* submitBuffers[1] = {commandBuffer.get()};
    SemaphoreValue signal{.semaphore = finished.get(), .value = 1};
    slagGraphicsCard()->graphicsQueue()->submit(submitBuffers,1,nullptr,0,&signal,1);
    finished->waitForValue(1);

    byteColor* dataPtr = textureBuffer->as<byteColor>();
    for (auto i=0; i< textureBuffer->countAsArray<byteColor>(); i++)
    {
        byteColor color = dataPtr[i];
        GTEST_ASSERT_EQ(color.r,255);
        GTEST_ASSERT_EQ(color.g,127);
        GTEST_ASSERT_EQ(color.b,50);
        GTEST_ASSERT_EQ(color.a,25);
    }
}

#ifdef SLAG_DEBUG
TEST(CommandBuffer, UpdateMipFailInRenderPass)
{
    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::GRAPHICS));
    std::unique_ptr<Semaphore> finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    struct byteColor
    {
        unsigned char r;
        unsigned char g;
        unsigned char b;
        unsigned char a;
    };
    std::vector<byteColor> texels(32*32,byteColor{255,127,50,25});
    std::unique_ptr<Texture> texture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::SAMPLED_IMAGE,32,32,1,2,texels.data(),1,1));
    std::unique_ptr<Texture> frameBuffer = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::SAMPLED_IMAGE,32,32,1,1));

    commandBuffer->begin();

    Attachment attachment{.texture = frameBuffer.get(),.autoClear = true};
    commandBuffer->beginRendering(&attachment,1,nullptr,slag::Rectangle{.offset = {0,0},.extent = {frameBuffer->width(),frameBuffer->height()}});
    ASSERT_DEATH(commandBuffer->updateMip(texture.get(),0,0,1),"Updated mip in renderpass");
}
#endif

TEST(CommandBuffer, CopyBufferToBuffer)
{
    std::vector<unsigned char> rawData(100);
    for (int i=0; i < 100; i++)
    {
        rawData[i] = i;
    }
    std::unique_ptr<Buffer> buffer1 = std::unique_ptr<Buffer>(Buffer::newBuffer(rawData.data(),rawData.size(),Buffer::Accessibility::CPU_AND_GPU));
    std::unique_ptr<Buffer> buffer2 = std::unique_ptr<Buffer>(Buffer::newBuffer(rawData.size(),Buffer::Accessibility::GPU));
    std::unique_ptr<Buffer> buffer3 = std::unique_ptr<Buffer>(Buffer::newBuffer(rawData.size(),Buffer::Accessibility::CPU_AND_GPU));

    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::GRAPHICS));
    std::unique_ptr<Semaphore> finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));

    commandBuffer->begin();

    commandBuffer->copyBufferToBuffer(buffer1.get(),0,50,buffer2.get(),50);
    commandBuffer->insertBarrier(BufferBarrier
        {
            .buffer = buffer2.get(),
            .accessBefore = BarrierAccessFlags::TRANSFER_WRITE,
            .accessAfter = BarrierAccessFlags::TRANSFER_READ,
            .syncBefore = PipelineStageFlags::TRANSFER,
            .syncAfter = PipelineStageFlags::TRANSFER
        });
    commandBuffer->copyBufferToBuffer(buffer2.get(),0,50,buffer3.get(),25);

    commandBuffer->end();

    CommandBuffer* submitBuffers[1] = {commandBuffer.get()};
    SemaphoreValue signal{.semaphore = finished.get(), .value = 1};
    slagGraphicsCard()->graphicsQueue()->submit(submitBuffers,1,nullptr,0,&signal,1);
    finished->waitForValue(1);

    unsigned char* dataPtr = buffer3->as<unsigned char>();
    for (auto i=0; i<50; i++)
    {
        GTEST_ASSERT_EQ(dataPtr[25+i],i);
    }
}

TEST(CommandBuffer, CopyTextureToBuffer)
{
    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::GRAPHICS));
    std::unique_ptr<Semaphore> finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    struct byteColor
    {
        unsigned char r;
        unsigned char g;
        unsigned char b;
        unsigned char a;
    };
    std::vector<byteColor> texels((32*32)*2+(16*16)*2,byteColor{255,0,255,255});
    for (int i=(32*32)+(16*16); i<texels.size(); i++)
    {
        texels[i] = byteColor{122,36,15,100};
    }
    std::unique_ptr<Texture> texture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::SAMPLED_IMAGE,32,32,2,2,texels.data(),2,2));
    std::unique_ptr<Buffer> textureBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(texture->byteSize(0)+texture->byteSize(1)+30,Buffer::Accessibility::CPU_AND_GPU));

    commandBuffer->begin();

    TextureToBufferCopyData copyData[]
    {
        {
            .bufferOffset = 15,
            .subresource =
        {
                .aspectFlags = Pixels::AspectFlags::COLOR,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            }
        },
        {
            .bufferOffset = texture->byteSize(0)+15,
            .subresource =
            {
                .aspectFlags = Pixels::AspectFlags::COLOR,
                .mipLevel = 1,
                .baseArrayLayer = 1,
                .layerCount = 1,
            }
        }
    };
    commandBuffer->copyTextureToBuffer(texture.get(),copyData,2,textureBuffer.get());

    commandBuffer->end();

    CommandBuffer* submitBuffers[1] = {commandBuffer.get()};
    SemaphoreValue signal{.semaphore = finished.get(), .value = 1};
    slagGraphicsCard()->graphicsQueue()->submit(submitBuffers,1,nullptr,0,&signal,1);
    finished->waitForValue(1);

    byteColor* colorPtr = textureBuffer->as<byteColor>();
    for (auto i=15; i < texture->byteSize(0)+15; i++)
    {
        GTEST_ASSERT_EQ(colorPtr[i].r,255);
        GTEST_ASSERT_EQ(colorPtr[i].g,0);
        GTEST_ASSERT_EQ(colorPtr[i].r,255);
        GTEST_ASSERT_EQ(colorPtr[i].r,255);
    }
    for (auto i=15+texture->byteSize(0); i < 15+texture->byteSize(0)+texture->byteSize(1); i++)
    {
        GTEST_ASSERT_EQ(colorPtr[i].r,255);
        GTEST_ASSERT_EQ(colorPtr[i].g,0);
        GTEST_ASSERT_EQ(colorPtr[i].r,255);
        GTEST_ASSERT_EQ(colorPtr[i].r,255);
    }
}

TEST(CommandBuffer, Blit)
{
    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::GRAPHICS));
    std::unique_ptr<Semaphore> finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    struct byteColor
    {
        unsigned char r;
        unsigned char g;
        unsigned char b;
        unsigned char a;
    };

    struct floatColor
    {
        float r;
        float g;
        float b;
        float a;
    };
    std::vector<floatColor> floatTexels(32*32,{1.0f,0,0,.5f});
    for (auto height=0; height < 32; height++)
    {
        for (auto width=0; width < 32; width++)
        {
            auto& texel = floatTexels[32*height+width];
            texel.g = ((float)height)/((float)32);
            texel.b = ((float)width)/((float)32);
        }
    }
    std::vector<byteColor> byteTexels(32*32,{0,255,0,255});

    std::unique_ptr<Texture> floatTexture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R32G32B32A32_FLOAT,Texture::Type::TEXTURE_2D,Texture::UsageFlags::SAMPLED_IMAGE,32,32,1,1,floatTexels.data(),1,1));
    std::unique_ptr<Texture> byteTexture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::SAMPLED_IMAGE,32,32,1,1,byteTexels.data(),1,1));
    std::unique_ptr<Buffer> textureBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(byteTexture->byteSize(),Buffer::Accessibility::CPU_AND_GPU));

    commandBuffer->begin();

    commandBuffer->blit(floatTexture.get(),0,0,slag::Rectangle{.offset = {16,16},.extent = {16,16}},byteTexture.get(),0,0,slag::Rectangle{.offset ={1,1},.extent = {30,30}});
    commandBuffer->insertBarrier(TextureBarrier
        {
            .texture = byteTexture.get(),
            .accessBefore = BarrierAccessFlags::BLIT_WRITE,
            .accessAfter = BarrierAccessFlags::TRANSFER_READ,
            .syncBefore = PipelineStageFlags::BLIT,
            .syncAfter = PipelineStageFlags::TRANSFER,
        });
    TextureToBufferCopyData copyData
    {
        .bufferOffset = 0,
        .subresource =
     {
            .aspectFlags = Pixels::AspectFlags::COLOR,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }
    };
    commandBuffer->copyTextureToBuffer(byteTexture.get(),&copyData,1,textureBuffer.get());

    commandBuffer->end();

    CommandBuffer* submitBuffers[1] = {commandBuffer.get()};
    SemaphoreValue signal{.semaphore = finished.get(), .value = 1};
    slagGraphicsCard()->graphicsQueue()->submit(submitBuffers,1,nullptr,0,&signal,1);
    finished->waitForValue(1);

    for (auto height = 1; height < 31; height++)
    {
        for (auto width = 1; width < 31; width++)
        {

        }
    }

    byteColor* colorPtr = textureBuffer->as<byteColor>();

    GTEST_FAIL();
}

TEST(CommandBuffer, Resolve)
{
    GTEST_FAIL();
}

TEST(CommandBuffer, FillBuffer)
{
    GTEST_FAIL();
}

TEST(CommandBuffer, SetBlendConstants)
{
    GTEST_FAIL();
}

TEST(CommandBuffer, SetStencilReference)
{
    GTEST_FAIL();
}

TEST(CommandBuffer, Draw)
{
    GTEST_FAIL();
}

TEST(CommandBuffer, DrawIndexed)
{
    GTEST_FAIL();
}

TEST(CommandBuffer, DrawIndexedIndirect)
{
    GTEST_FAIL();
}

TEST(CommandBuffer, DrawIndexedIndirectCount)
{
    GTEST_FAIL();
}

TEST(CommandBuffer, DrawIndirect)
{
    GTEST_FAIL();
}

TEST(CommandBuffer, DrawIndirectCount)
{
    GTEST_FAIL();
}

TEST(CommandBuffer, Dispatch)
{
    GTEST_FAIL();
}

TEST(CommandBuffer, DispatchBase)
{
    GTEST_FAIL();
}

TEST(CommandBuffer, DispatchIndirect)
{
    GTEST_FAIL();
}
#endif