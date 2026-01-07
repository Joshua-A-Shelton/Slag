#include <gtest/gtest.h>
#include <slag/Slag.h>
#include "../Utilities.h"
#include "../GraphicsAPIEnvironment.h"
#include "../third-party/LodePNG/lodepng.h"

using namespace slag;
float DESCRIPTOR_MEMORY_TRIANGLE_DATA[]={-1.0f,-1.0f,0,1.0f,1.0f,-1.0f};
float DESCRIPTOR_MEMORY_UV_DATA[]={0,0,.5f,1.0f,0,1.0f};

TEST(DescriptorMemory, ResourceDescriptorGroupOffset)
{
    if (slagGraphicsCard()->descriptorBufferOffsetAlignment() > 0)
    {
        auto resourceMemory = std::unique_ptr<ResourceDescriptorMemory>(ResourceDescriptorMemory::newResourceDescriptorMemory(1000));
        GTEST_ASSERT_EQ(resourceMemory->descriptorGroupOffset(0),0);
        GTEST_ASSERT_EQ(resourceMemory->descriptorGroupOffset(1),slagGraphicsCard()->descriptorBufferOffsetAlignment());
        GTEST_ASSERT_EQ(resourceMemory->descriptorGroupOffset(slagGraphicsCard()->descriptorBufferOffsetAlignment()+1),slagGraphicsCard()->descriptorBufferOffsetAlignment()*2);
        GTEST_ASSERT_EQ(resourceMemory->descriptorGroupOffset(slagGraphicsCard()->descriptorBufferOffsetAlignment()),slagGraphicsCard()->descriptorBufferOffsetAlignment());
        GTEST_ASSERT_EQ(resourceMemory->descriptorGroupOffset(slagGraphicsCard()->descriptorBufferOffsetAlignment()*2),slagGraphicsCard()->descriptorBufferOffsetAlignment()*2);
    }
    else
    {
        GTEST_SKIP();
    }
}
TEST(DescriptorMemory, SamplerDescriptorGroupOffset)
{
    if (slagGraphicsCard()->descriptorBufferOffsetAlignment() > 0)
    {
        auto samplerMemory = std::unique_ptr<SamplerDescriptorMemory>(SamplerDescriptorMemory::newSamplerDescriptorMemory(1000));
        GTEST_ASSERT_EQ(samplerMemory->descriptorGroupOffset(0),0);
        GTEST_ASSERT_EQ(samplerMemory->descriptorGroupOffset(1),slagGraphicsCard()->descriptorBufferOffsetAlignment());
        GTEST_ASSERT_EQ(samplerMemory->descriptorGroupOffset(slagGraphicsCard()->descriptorBufferOffsetAlignment()+1),slagGraphicsCard()->descriptorBufferOffsetAlignment()*2);
        GTEST_ASSERT_EQ(samplerMemory->descriptorGroupOffset(slagGraphicsCard()->descriptorBufferOffsetAlignment()),slagGraphicsCard()->descriptorBufferOffsetAlignment());
        GTEST_ASSERT_EQ(samplerMemory->descriptorGroupOffset(slagGraphicsCard()->descriptorBufferOffsetAlignment()*2),slagGraphicsCard()->descriptorBufferOffsetAlignment()*2);
    }
    else
    {
        GTEST_SKIP();
    }
}
TEST(DescriptorMemory, SetSampledTexture)
{
    ShaderFile stages[] =
    {
        {
            .pathIndicator = "resources/shaders/SetSampledTexture.vertex",
            .stage = ShaderStageFlags::VERTEX,
        },
        {
            .pathIndicator = "resources/shaders/SetSampledTexture.fragment",
            .stage = ShaderStageFlags::FRAGMENT,
        }
    };
    ShaderProperties properties{};
    VertexDescription vertexDescription(2);
    vertexDescription.add(GraphicsType::VECTOR2, 0, 0);
    vertexDescription.add(GraphicsType::VECTOR2, 0, 1);
    FrameBufferDescription frameBufferDescription;
    frameBufferDescription.colorTargets[0] = Pixels::Format::R8G8B8A8_UNORM;


    auto pipeline = GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(stages, 2, properties, vertexDescription, frameBufferDescription);
    auto finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    auto triangleData = std::unique_ptr<Buffer>(Buffer::newBuffer(DESCRIPTOR_MEMORY_TRIANGLE_DATA,sizeof(float)*6,Buffer::Accessibility::GPU,Buffer::UsageFlags::VERTEX_BUFFER));
    auto uvData = std::unique_ptr<Buffer>(Buffer::newBuffer(DESCRIPTOR_MEMORY_UV_DATA,sizeof(float)*6,Buffer::Accessibility::GPU,Buffer::UsageFlags::VERTEX_BUFFER));
    auto texture = utilities::loadTextureFromFile("resources/textures/gradient.jpg");
    auto target = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::RENDER_TARGET_ATTACHMENT,64,64,1,1,1));
    auto texturePixels = std::unique_ptr<Buffer>(Buffer::newBuffer(texture->byteSize(Pixels::AspectFlags::COLOR),Buffer::Accessibility::CPU_AND_GPU));
    auto sampler = std::unique_ptr<Sampler>(Sampler::newSampler(SamplerParameters{}));
    auto resourceMemory = std::unique_ptr<ResourceDescriptorMemory>(ResourceDescriptorMemory::newResourceDescriptorMemory(1000));
    auto samplerMemory = std::unique_ptr<SamplerDescriptorMemory>(SamplerDescriptorMemory::newSamplerDescriptorMemory(1000));
    auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::GRAPHICS));

    resourceMemory->setSampledTexture(0+pipeline->descriptorGroup(1)->descriptorByteOffset(0),texture.get());
    samplerMemory->setSampler(0+pipeline->descriptorGroup(0)->descriptorByteOffset(0),sampler.get());
    commandBuffer->begin();
    commandBuffer->setViewPort(0,0,target->width(),target->height(),0,1);
    commandBuffer->setScissors(slag::Rectangle{{0,0},{target->width(),target->height()}});
    Attachment attachment{.texture = target.get(),.autoClear=true,.clearValue=ClearColor{0,0,0,1.0f}};
    commandBuffer->beginRendering(&attachment,1,nullptr,slag::Rectangle{{0,0},{target->width(),target->height()}});
    commandBuffer->bindDescriptorMemory(resourceMemory.get(),samplerMemory.get());
    commandBuffer->bindGraphicsShaderPipeline(pipeline.get());
    commandBuffer->bindGraphicsDescriptorGroup(0,DescriptorGroup::DescriptorMemory::SAMPLER,0);
    commandBuffer->bindGraphicsDescriptorGroup(1,DescriptorGroup::DescriptorMemory::RESOURCE,0);
    uint64_t bufferOffset = 0;
    uint64_t stride = sizeof(float)*2;
    auto trianglePtr = triangleData.get();
    auto uvPtr = uvData.get();
    commandBuffer->bindVertexBuffers(0,&trianglePtr,&bufferOffset,&stride,1);
    commandBuffer->bindVertexBuffers(1,&uvPtr,&bufferOffset,&stride,1);
    commandBuffer->draw(3,1,0,0);
    commandBuffer->endRendering();
    commandBuffer->insertBarrier(TextureBarrier
        {
            .texture = texture.get(),
            .baseLayer = 0,
            .layerCount = 1,
            .baseMipLevel = 0,
            .mipCount = 1,
            .accessBefore = BarrierAccessFlags::NONE,
            .accessAfter = BarrierAccessFlags::TRANSFER_READ,
            .syncBefore = PipelineStageFlags::FRAGMENT_SHADER,
            .syncAfter = PipelineStageFlags::TRANSFER
        });
    TextureBufferMapping mapping
    {
        .bufferOffset = 0,
        .textureSubresource =
        {
            .aspectFlags = Pixels::AspectFlags::COLOR,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .textureOffset = {0,0,0},
        .textureExtent = {texture->width(),texture->height(),1},
    };
    commandBuffer->copyTextureToBuffer(target.get(), texturePixels.get(),&mapping,1);
    commandBuffer->end();

    SemaphoreValue signal{.semaphore = finished.get(),.value =1};
    auto cmdBuffer = commandBuffer.get();
    QueueSubmissionBatch submissionBatch
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = &cmdBuffer,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1,
    };

    slagGraphicsCard()->graphicsQueue()->submit(&submissionBatch,1);
    finished->waitForValue(1);


    GTEST_FAIL();
}
TEST(DescriptorMemory, SetStorageTexture)
{
    GTEST_FAIL();
}
TEST(DescriptorMemory, SetUniformTexelBuffer)
{
    GTEST_FAIL();
}
TEST(DescriptorMemory, SetStorageTexelBuffer)
{
    GTEST_FAIL();
}
TEST(DescriptorMemory, SetUniformBuffer)
{
    GTEST_FAIL();
}
TEST(DescriptorMemory, SetStorageBuffer)
{
    GTEST_FAIL();
}
TEST(DescriptorMemory, SetSampler)
{
    GTEST_FAIL();
}