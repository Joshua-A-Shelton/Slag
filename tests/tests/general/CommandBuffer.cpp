#ifndef SLAG_DISCREET_TEXTURE_LAYOUTS
#include "../../GraphicsAPIEnvironment.h"
#include "slag/core/DescriptorBundle.h"
#include "slag/core/DescriptorPool.h"
#include <gtest/gtest.h>
#include <slag/Slag.h>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../../Utilities.h"
#include "../../third-party/LodePNG/lodepng.h"
using namespace slag;

struct GlobalSet0Group
{
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 projectionView;
};
struct TexturedDepthSet1Group
{
    glm::mat4 position;
};

class CommandBufferTest: public ::testing::Test
{
protected:
    std::unique_ptr<Buffer> TriangleVerts;
    std::unique_ptr<Buffer> TriangleUVs;
    std::unique_ptr<Buffer> TriangleNormals;
    std::unique_ptr<Buffer> TriangleIndices;
    std::unique_ptr<Buffer> CubeVerts;
    std::unique_ptr<Buffer> CubeUVs;
    std::unique_ptr<Buffer> CubeIndices;

    std::unique_ptr<Buffer> CubeVertsRaw;
    std::unique_ptr<Buffer> CubeUVsRaw;

    VertexDescription VertexPosDescription = VertexDescription(1);
    VertexDescription VertexPosUVDescription = VertexDescription(2);

    std::unique_ptr<ShaderPipeline> TexturedDepthPipeline;
    std::unique_ptr<ShaderPipeline> TexturedDepthMultiSamplePipeline;
    std::unique_ptr<Sampler> DefaultSampler;

    CommandBufferTest()
    {
        std::vector<glm::vec3> tverts = {{ -1.f, -1.f, 0.0f},{0.f,1.f, 0.0f},{1.f, -1.f, 0.0f}};
        std::vector<glm::vec2> tuvs = {{0,1},{.5,0},{1,1}};
        std::vector<glm::vec3> tnormals = {{0,0,1},{0,0,1},{0,0,1}};
        std::vector<uint16_t> tindexes = {0,1,2};

        TriangleVerts = std::unique_ptr<Buffer>(Buffer::newBuffer(tverts.data(),tverts.size()*sizeof(glm::vec3),Buffer::Accessibility::GPU,Buffer::UsageFlags::VERTEX_BUFFER));
        TriangleUVs = std::unique_ptr<Buffer>(Buffer::newBuffer(tuvs.data(),tuvs.size()*sizeof(glm::vec2),Buffer::Accessibility::GPU,Buffer::UsageFlags::VERTEX_BUFFER));
        TriangleNormals = std::unique_ptr<Buffer>(Buffer::newBuffer(tnormals.data(),tnormals.size()*sizeof(glm::vec3),Buffer::Accessibility::GPU,Buffer::UsageFlags::VERTEX_BUFFER));
        TriangleIndices = std::unique_ptr<Buffer>(Buffer::newBuffer(tindexes.data(),tindexes.size()*sizeof(uint16_t),Buffer::Accessibility::GPU,Buffer::UsageFlags::INDEX_BUFFER));

        std::vector<float> cverts =
        {
            -0.5f, 0.5f, -0.5f,  // A 0
            0.5f, 0.5f, -0.5f,   // B 1
            0.5f,  -0.5f, -0.5f,   // C 2
            -0.5f,  -0.5f, -0.5f,  // D 3
            -0.5f, 0.5f,  0.5f,  // E 4
            0.5f, 0.5f,  0.5f,   // F 5
            0.5f,  -0.5f,  0.5f,   // G 6
            -0.5f,  -0.5f,  0.5f,   // H 7

            -0.5f,  -0.5f, -0.5f,  // D 8
            -0.5f, 0.5f, -0.5f,  // A 9
            -0.5f, 0.5f,  0.5f,  // E 10
            -0.5f,  -0.5f,  0.5f,   // H 11
            0.5f, 0.5f, -0.5f,   // B 12
            0.5f,  -0.5f, -0.5f,   // C 13
            0.5f,  -0.5f,  0.5f,   // G 14
            0.5f, 0.5f,  0.5f,   // F 15

            -0.5f, 0.5f, -0.5f,   // A 16
            0.5f, 0.5f, -0.5f,    // B 17
            0.5f, 0.5f,  0.5f,    // F 18
            -0.5f, 0.5f,  0.5f,   // E 19
            0.5f,  -0.5f, -0.5f,   // C 20
            -0.5f,  -0.5f, -0.5f, // D 21
            -0.5f,  -0.5f,  0.5f,  // H 22
            0.5f,  -0.5f,  0.5f,   // G 23
        };
        std::vector<float> cuvs =
        {
            0.0f, 0.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
            0.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
            0.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
            0.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
            0.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
            0.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
            0.0f, 1.0f,
        };

        std::vector<uint16_t> cindexes =
        {
            0, 3, 2,
             2, 1, 0,
             4, 5, 6,
             6, 7 ,4,
             // left and right
             11, 8, 9,
             9, 10, 11,
             12, 13, 14,
             14, 15, 12,
             // bottom and top
             16, 17, 18,
             18, 19, 16,
             20, 21, 22,
             22, 23, 20
        };

        std::vector<float> cvertsRaw(cindexes.size()*3);
        std::vector<float> cuvsRaw(cindexes.size()*2);
        for (auto i=0, j=0, k=0; i<cindexes.size(); i++, j+=3, k+=2)
        {
            cvertsRaw[j] = cverts[cindexes[i]];
            cvertsRaw[j+1] = cverts[cindexes[i]+1];
            cvertsRaw[j+2] = cverts[cindexes[i]+2];

            cuvsRaw[k] = cuvs[cindexes[i]];
            cuvsRaw[k+1] = cuvs[cindexes[i]+1];
        }

        CubeVerts = std::unique_ptr<Buffer>(Buffer::newBuffer(cverts.data(),cverts.size()*sizeof(float),Buffer::Accessibility::GPU,Buffer::UsageFlags::VERTEX_BUFFER));
        CubeUVs = std::unique_ptr<Buffer>(Buffer::newBuffer(cuvs.data(),cuvs.size()*sizeof(float),Buffer::Accessibility::GPU,Buffer::UsageFlags::VERTEX_BUFFER));

        CubeVertsRaw = std::unique_ptr<Buffer>(Buffer::newBuffer(cvertsRaw.data(),cvertsRaw.size()*sizeof(float),Buffer::Accessibility::GPU,Buffer::UsageFlags::VERTEX_BUFFER));
        CubeUVsRaw = std::unique_ptr<Buffer>(Buffer::newBuffer(cuvsRaw.data(),cuvsRaw.size()*sizeof(float),Buffer::Accessibility::GPU,Buffer::UsageFlags::VERTEX_BUFFER));

        CubeIndices = std::unique_ptr<Buffer>(Buffer::newBuffer(cindexes.data(),cindexes.size()*sizeof(uint16_t),Buffer::Accessibility::GPU,Buffer::UsageFlags::INDEX_BUFFER));

        VertexPosDescription.add(GraphicsType::VECTOR3,0,0);
        VertexPosUVDescription.add(GraphicsType::VECTOR3,0,0).add(GraphicsType::VECTOR2,0,1);

        std::vector<ShaderFile> shaderFiles =
        {
            ShaderFile("resources/shaders/TexturedDepth.vertex",ShaderStageFlags::VERTEX),
            ShaderFile("resources/shaders/TexturedDepth.fragment",ShaderStageFlags::FRAGMENT)
        };
        ShaderProperties properties;
        FrameBufferDescription framebufferDescription;
        framebufferDescription.colorTargets[0] = Pixels::Format::R8G8B8A8_UNORM;
        framebufferDescription.depthTarget = Pixels::Format::D24_UNORM_S8_UINT;
        TexturedDepthPipeline = GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(shaderFiles.data(),shaderFiles.size(),properties,VertexPosUVDescription,framebufferDescription);
        properties.multiSampleState.rasterizationSamples = 4;
        TexturedDepthMultiSamplePipeline = GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(shaderFiles.data(),shaderFiles.size(),properties,VertexPosUVDescription,framebufferDescription);
        DefaultSampler = std::unique_ptr<Sampler>(Sampler::newSampler(SamplerParameters()));
    }

};

TEST_F(CommandBufferTest, ClearColor)
{
    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::GRAPHICS));
    std::unique_ptr<Semaphore> finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore());
    std::unique_ptr<Texture> renderTarget = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::RENDER_TARGET_ATTACHMENT,32,32,1,1,1));
    std::unique_ptr<Texture> sampled = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::SAMPLED_IMAGE,32,32,1,4,1));
    std::unique_ptr<Buffer> rtBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(renderTarget->byteSize(),Buffer::Accessibility::CPU_AND_GPU));
    std::unique_ptr<Buffer> sampledBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(sampled->byteSize(),Buffer::Accessibility::CPU_AND_GPU));

    commandBuffer->begin();

    commandBuffer->clearTexture(renderTarget.get(),ClearColor{.floats = {1.0f,0.0f,0.0f,1.0f}});
    commandBuffer->clearTexture(sampled.get(),ClearColor{.floats = {0.0f,1.0f,0.0f,1.0f}});


    TextureBarrier barriers[2]
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

    };
    commandBuffer->insertBarriers(barriers,2,nullptr,0,nullptr,0);
    TextureBufferMapping rtCopyData
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
       .textureExtent = {renderTarget->width(),renderTarget->height(),renderTarget->depth()}
   };
    commandBuffer->copyTextureToBuffer(renderTarget.get(),rtBuffer.get(),&rtCopyData,1);

    TextureBufferMapping sampledCopyData[4]
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
            .textureExtent = {sampled->width(),sampled->height(),1}
        },
        {
            .bufferOffset = sampled->byteSize(0),
            .textureSubresource =
            {
                .aspectFlags = Pixels::AspectFlags::COLOR,
                .mipLevel = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .textureOffset = {0,0,0},
            .textureExtent = {sampled->width(1),sampled->height(1),1}
        },
        {
            .bufferOffset = sampled->byteSize(0)+sampled->byteSize(1),
            .textureSubresource =
         {
                .aspectFlags = Pixels::AspectFlags::COLOR,
                .mipLevel = 2,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .textureOffset = {0,0,0},
            .textureExtent = {sampled->width(2),sampled->height(2),1}
        },
        {
            .bufferOffset = sampled->byteSize(0)+sampled->byteSize(1)+sampled->byteSize(2),
            .textureSubresource =
        {
                .aspectFlags = Pixels::AspectFlags::COLOR,
                .mipLevel = 3,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .textureOffset = {0,0,0},
            .textureExtent = {sampled->width(3),sampled->height(3),1}
        }
    };
    commandBuffer->copyTextureToBuffer(sampled.get(),sampledBuffer.get(),sampledCopyData,4);

    commandBuffer->end();

    CommandBuffer* submitBuffers[1] = {commandBuffer.get()};
    SemaphoreValue signal{.semaphore = finished.get(), .value = 1};
    QueueSubmissionBatch submissionData
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = submitBuffers,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1,
    };
    slag::slagGraphicsCard()->graphicsQueue()->submit(&submissionData,1);
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
}
#ifdef SLAG_DEBUG
TEST_F(CommandBufferTest, ClearColorFailInRenderPass)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::GRAPHICS));
    std::unique_ptr<Semaphore> finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    std::unique_ptr<Texture> texture1 = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::RENDER_TARGET_ATTACHMENT,32,32,1,1,1));
    std::unique_ptr<Texture> texture2 = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::RENDER_TARGET_ATTACHMENT,32,32,1,1,1));
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

TEST_F(CommandBufferTest, ClearDepth)
{
    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::GRAPHICS));
    std::unique_ptr<Semaphore> finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    std::unique_ptr<Texture> depthTexture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::D32_FLOAT,Texture::Type::TEXTURE_2D,Texture::UsageFlags::DEPTH_STENCIL_ATTACHMENT,32,32,1,1,1));
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
    TextureBufferMapping copyData
    {
        .bufferOffset = 0,
        .textureSubresource =
     {
            .aspectFlags = Pixels::AspectFlags::DEPTH,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
         .textureOffset = {0,0,0},
         .textureExtent = {depthTexture->width(),depthTexture->height(),1}
    };
    commandBuffer->copyTextureToBuffer(depthTexture.get(),depthBuffer.get(),&copyData,1);
    commandBuffer->end();

    CommandBuffer* submitBuffers[1] = {commandBuffer.get()};
    SemaphoreValue signal{.semaphore = finished.get(), .value = 1};
    QueueSubmissionBatch submissionData
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = submitBuffers,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1,
    };
    slag::slagGraphicsCard()->graphicsQueue()->submit(&submissionData,1);
    finished->waitForValue(1);

    auto depthPtr = depthBuffer->as<float>();
    for (auto i=0; i< depthBuffer->countAsArray<float>(); i++)
    {
        GTEST_ASSERT_EQ(depthPtr[i],1.0f);
    }
}

#ifdef SLAG_DEBUG
TEST_F(CommandBufferTest, ClearDepthFailInRenderPass)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::GRAPHICS));
    std::unique_ptr<Semaphore> finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    std::unique_ptr<Texture> color = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::RENDER_TARGET_ATTACHMENT,32,32,1,1,1));
    std::unique_ptr<Texture> texture1 = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::D32_FLOAT,Texture::Type::TEXTURE_2D,Texture::UsageFlags::DEPTH_STENCIL_ATTACHMENT,32,32,1,1,1));
    std::unique_ptr<Texture> texture2 = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::D24_UNORM_S8_UINT,Texture::Type::TEXTURE_2D,Texture::UsageFlags::DEPTH_STENCIL_ATTACHMENT,32,32,1,1,1));
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

TEST_F(CommandBufferTest, UpdateMip)
{
    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::GRAPHICS));
    std::unique_ptr<Semaphore> finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    struct byteColor
    {
        unsigned char r;
        unsigned char g;
        unsigned char b;
        unsigned char a;
    };
    std::vector<byteColor> texels(32*32,byteColor{255,127,50,25});
    TextureBufferMapping mappings[]
    {
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
            .textureExtent = {32,32,1}
        },
    {
            .bufferOffset = 32*32,
            .textureSubresource =
        {
                .aspectFlags = Pixels::AspectFlags::COLOR,
                .mipLevel = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .textureOffset = {0,0,0},
            .textureExtent = {16,16,1}
        }
    };

    std::unique_ptr<Texture> texture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::SAMPLED_IMAGE,32,32,1,2,1,Texture::SampleCount::ONE,texels.data(),texels.size()*Pixels::size(Pixels::Format::R8G8B8A8_UNORM),mappings,2));
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
    TextureBufferMapping copyData
    {
        .bufferOffset = 0,
        .textureSubresource =
        {
            .aspectFlags = Pixels::AspectFlags::COLOR,
            .mipLevel = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
         .textureOffset = {0,0,0},
         .textureExtent = {texture->width(1),texture->height(1),1}
    };
    commandBuffer->copyTextureToBuffer(texture.get(),textureBuffer.get(),&copyData,1);

    commandBuffer->end();

    CommandBuffer* submitBuffers[1] = {commandBuffer.get()};
    SemaphoreValue signal{.semaphore = finished.get(), .value = 1};
    QueueSubmissionBatch submissionData
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = submitBuffers,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1,
    };
    slagGraphicsCard()->graphicsQueue()->submit(&submissionData,1);
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
TEST_F(CommandBufferTest, UpdateMipFailInRenderPass)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::GRAPHICS));
    std::unique_ptr<Semaphore> finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    struct byteColor
    {
        unsigned char r;
        unsigned char g;
        unsigned char b;
        unsigned char a;
    };
    std::vector<byteColor> texels(32*32,byteColor{255,127,50,25});
    TextureBufferMapping mappings[]
    {
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
            .textureExtent = {32,32,1}
        },
    {
        .bufferOffset = 32*32,
        .textureSubresource =
    {
            .aspectFlags = Pixels::AspectFlags::COLOR,
            .mipLevel = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .textureOffset = {0,0,0},
        .textureExtent = {16,16,1}
    }
    };
    std::unique_ptr<Texture> texture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::SAMPLED_IMAGE,32,32,1,2,1,Texture::SampleCount::ONE,texels.data(),texels.size()*sizeof(byteColor),mappings,2));
    std::unique_ptr<Texture> frameBuffer = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::RENDER_TARGET_ATTACHMENT,32,32,1,1,1));

    commandBuffer->begin();

    Attachment attachment{.texture = frameBuffer.get(),.autoClear = true};
    commandBuffer->beginRendering(&attachment,1,nullptr,slag::Rectangle{.offset = {0,0},.extent = {frameBuffer->width(),frameBuffer->height()}});
    ASSERT_DEATH(commandBuffer->updateMip(texture.get(),0,0,1),"Updated mip in renderpass");
}
#endif

TEST_F(CommandBufferTest, CopyBufferToBuffer )
{
    std::vector<unsigned char> rawData(100);
    for (int i=0; i < 100; i++)
    {
        rawData[i] = i;
    }
    std::vector<unsigned char> fixed(100,255);
    std::unique_ptr<Buffer> buffer1 = std::unique_ptr<Buffer>(Buffer::newBuffer(rawData.data(),rawData.size(),Buffer::Accessibility::CPU_AND_GPU));
    std::unique_ptr<Buffer> buffer2 = std::unique_ptr<Buffer>(Buffer::newBuffer(rawData.size(),Buffer::Accessibility::GPU));
    std::unique_ptr<Buffer> buffer3 = std::unique_ptr<Buffer>(Buffer::newBuffer(fixed.data(),fixed.size(),Buffer::Accessibility::CPU_AND_GPU));

    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::GRAPHICS));
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
    commandBuffer->copyBufferToBuffer(buffer2.get(),50,50,buffer3.get(),25);

    commandBuffer->end();

    CommandBuffer* submitBuffers[1] = {commandBuffer.get()};
    SemaphoreValue signal{.semaphore = finished.get(), .value = 1};
    QueueSubmissionBatch submissionData
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = submitBuffers,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1,
    };
    slagGraphicsCard()->graphicsQueue()->submit(&submissionData,1);
    finished->waitForValue(1);

    unsigned char* dataPtr = buffer3->as<unsigned char>();
    for (auto i=0; i<50; i++)
    {
        GTEST_ASSERT_EQ(dataPtr[25+i],i);
    }
}

TEST_F(CommandBufferTest, CopyTextureToBuffer)
{
    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::GRAPHICS));
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
    TextureBufferMapping mappings[]
    {
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
            .textureExtent = {32,32,1}
        },
    {
            .bufferOffset = 32*32*sizeof(byteColor),
            .textureSubresource =
        {
                .aspectFlags = Pixels::AspectFlags::COLOR,
                .mipLevel = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .textureOffset = {0,0,0},
            .textureExtent = {16,16,1}
        },
    {
            .bufferOffset = (32*32+16*16)*sizeof(byteColor),
            .textureSubresource =
         {
                .aspectFlags = Pixels::AspectFlags::COLOR,
                .mipLevel = 0,
                .baseArrayLayer = 1,
                .layerCount = 1,
            },
            .textureOffset = {0,0,0},
            .textureExtent = {32,32,1}
        },
        {
            .bufferOffset = ((32*32)*2+16*16)*sizeof(byteColor),
            .textureSubresource =
        {
                .aspectFlags = Pixels::AspectFlags::COLOR,
                .mipLevel = 1,
                .baseArrayLayer = 1,
                .layerCount = 1,
            },
            .textureOffset = {0,0,0},
            .textureExtent = {16,16,1}
        }
    };
    std::unique_ptr<Texture> texture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::SAMPLED_IMAGE,32,32,1,2,2,Texture::SampleCount::ONE,texels.data(),texels.size()*sizeof(byteColor),mappings,4));
    std::unique_ptr<Buffer> textureBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(texture->byteSize(0)+texture->byteSize(1)+32,Buffer::Accessibility::CPU_AND_GPU));

    commandBuffer->begin();

    TextureBufferMapping copyData[]
    {
        {
            .bufferOffset = 16,
            .textureSubresource =
        {
                .aspectFlags = Pixels::AspectFlags::COLOR,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
             .textureOffset = {0,0,0},
             .textureExtent = {texture->width(),texture->height(),1}
        },
        {
            .bufferOffset = texture->byteSize(0)+16,
            .textureSubresource =
            {
                .aspectFlags = Pixels::AspectFlags::COLOR,
                .mipLevel = 1,
                .baseArrayLayer = 1,
                .layerCount = 1,
            },
            .textureOffset = {0,0,0},
            .textureExtent = {texture->width(1),texture->height(1),1}
        }
    };
    commandBuffer->copyTextureToBuffer(texture.get(),textureBuffer.get(),copyData,2);

    commandBuffer->end();

    CommandBuffer* submitBuffers[1] = {commandBuffer.get()};
    SemaphoreValue signal{.semaphore = finished.get(), .value = 1};
    QueueSubmissionBatch submissionData
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = submitBuffers,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1,
    };
    slagGraphicsCard()->graphicsQueue()->submit(&submissionData,1);
    finished->waitForValue(1);

    uint8_t* colorPtr = textureBuffer->as<uint8_t>();
    for (auto i=16; i < texture->byteSize(0)+16; i+=4)
    {
        auto color = *reinterpret_cast<byteColor*>(&colorPtr[i]);
        GTEST_ASSERT_EQ(color.r,255);
        GTEST_ASSERT_EQ(color.g,0);
        GTEST_ASSERT_EQ(color.b,255);
        GTEST_ASSERT_EQ(color.a,255);
    }

    for (auto i=16+texture->byteSize(0); i < 16+texture->byteSize(0)+texture->byteSize(1); i+=4)
    {
        auto color = *reinterpret_cast<byteColor*>(&colorPtr[i]);
        GTEST_ASSERT_EQ(color.r,122);
        GTEST_ASSERT_EQ(color.g,36);
        GTEST_ASSERT_EQ(color.b,15);
        GTEST_ASSERT_EQ(color.a,100);
    }

}

TEST_F(CommandBufferTest, Blit)
{
    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::GRAPHICS));
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
        .textureExtent = {32,32,1}
    };
    std::unique_ptr<Texture> floatTexture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R32G32B32A32_FLOAT,Texture::Type::TEXTURE_2D,Texture::UsageFlags::SAMPLED_IMAGE,32,32,1,1,1,Texture::SampleCount::ONE,floatTexels.data(),floatTexels.size()*sizeof(floatColor),&mapping,1));
    std::unique_ptr<Texture> byteTexture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::SAMPLED_IMAGE,32,32,1,1,1,Texture::SampleCount::ONE, byteTexels.data(),byteTexels.size()*sizeof(byteColor),&mapping,1));
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
    TextureBufferMapping copyData
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
         .textureExtent = {byteTexture->width(0),byteTexture->height(0),1}
    };
    commandBuffer->copyTextureToBuffer(byteTexture.get(),textureBuffer.get(),&copyData,1);

    commandBuffer->end();

    CommandBuffer* submitBuffers[1] = {commandBuffer.get()};
    SemaphoreValue signal{.semaphore = finished.get(), .value = 1};
    QueueSubmissionBatch submissionData
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = submitBuffers,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1,
    };
    slagGraphicsCard()->graphicsQueue()->submit(&submissionData,1);
    finished->waitForValue(1);

    unsigned char* colorPtr = textureBuffer->as<unsigned char>();
    auto groundTruth = utilities::loadTexelsFromFile("resources/textures/blit-test-result.png");
    GTEST_ASSERT_EQ(textureBuffer->countAsArray<unsigned char>(),groundTruth.size());

    for (auto i=0; i< textureBuffer->countAsArray<unsigned char>(); i++)
    {
        //allow for less than 5% variation, hardware will vary slightly
        GTEST_ASSERT_TRUE(std::abs((int)colorPtr[i] - (int)groundTruth[i]) < 10);
    }
}

TEST_F(CommandBufferTest, Resolve)
{
    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::GRAPHICS));
    std::unique_ptr<Semaphore> finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    std::unique_ptr<Texture> multiSampled = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::RENDER_TARGET_ATTACHMENT,150,150,1,1,1,Texture::SampleCount::FOUR));
    std::unique_ptr<Texture> depth = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::D24_UNORM_S8_UINT,Texture::Type::TEXTURE_2D,Texture::UsageFlags::DEPTH_STENCIL_ATTACHMENT,150,150,1,1,1));
    std::unique_ptr<DescriptorPool> descriptorPool = std::unique_ptr<DescriptorPool>(DescriptorPool::newDescriptorPool());
    std::unique_ptr<Buffer> globalsBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(GlobalSet0Group),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_BUFFER));
    std::unique_ptr<Buffer> objectBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(TexturedDepthSet1Group),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_BUFFER));
    std::unique_ptr<Texture> objectTexture = utilities::loadTextureFromFile("resources/textures/gradient.jpg");
    std::unique_ptr<Texture> output = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::RENDER_TARGET_ATTACHMENT,150,150,1,1,1));
    std::unique_ptr<Buffer> outputBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(output->byteSize(),Buffer::Accessibility::CPU_AND_GPU));



    commandBuffer->begin();

    commandBuffer->bindDescriptorPool(descriptorPool.get());
    Attachment attachment = {.texture = multiSampled.get(),.autoClear = true,.clearValue = {.color = {.floats = {1,0,.5,1}}}};

    auto globalBundle = descriptorPool->makeBundle(TexturedDepthPipeline->descriptorGroup(0));
    auto objectBundle = descriptorPool->makeBundle(TexturedDepthPipeline->descriptorGroup(1));
    auto globals = globalsBuffer->as<GlobalSet0Group>();
    auto proj = glm::perspective(95.0f,(float)multiSampled->width()/(float)multiSampled->height(),.01f,100.0f);
    glm::mat4 view = glm::mat4(1.0f);
    view = glm::translate(view,glm::vec3(0.0f,2.0f,5.0f));
    view = glm::rotate(view,glm::radians(-20.0f),glm::vec3(1.0f,0.0f,0.0f));
    view = glm::inverse(view);//does nothing in this case, but is good practice if we ever do have a camera not at the default location
    glm::mat4 projectionView = proj*view;
    globals->projection = proj;
    globals->view = view;
    globals->projectionView = projectionView;
    auto object = objectBuffer->as<TexturedDepthSet1Group>();
    object->position = glm::rotate(glm::mat4(1.0f),glm::radians(45.0f),glm::vec3(0.0f,1.0f,0.0f));
    globalBundle.setUniformBuffer(0,0,globalsBuffer.get(),0,sizeof(GlobalSet0Group));
    commandBuffer->bindGraphicsShaderPipeline(TexturedDepthMultiSamplePipeline.get());
    commandBuffer->bindGraphicsDescriptorBundle(0,globalBundle);
    objectBundle.setUniformBuffer(0,0,objectBuffer.get(),0,sizeof(TexturedDepthSet1Group));
    objectBundle.setSampledTexture(1,0,objectTexture.get());
    objectBundle.setSampler(2,0,DefaultSampler.get());
    commandBuffer->bindGraphicsDescriptorBundle(0,globalBundle);
    commandBuffer->bindGraphicsDescriptorBundle(1,objectBundle);
    Attachment colorAttachment{.texture = multiSampled.get(),.autoClear = true,.clearValue = ClearValue{.color = {.floats = {0,0,0,1}}}};
    Attachment depthAttachment{.texture = depth.get(),.autoClear = true,.clearValue = ClearValue{.depthStencil = {.depth = 1, .stencil = 0}}};
    commandBuffer->beginRendering(&attachment,1,nullptr,slag::Rectangle{.offset = {0,0},.extent = {multiSampled->width(),multiSampled->height()}});

    Buffer* vertexBuffers[]
    {
        CubeVerts.get(),
        CubeUVs.get()
    };
    uint64_t vertexOffsets[]{0,0};
    uint64_t bufferStrides[2] = {sizeof(glm::vec3),sizeof(glm::vec2)};
    commandBuffer->bindVertexBuffers(0,vertexBuffers,vertexOffsets,bufferStrides,2);
    commandBuffer->bindIndexBuffer(CubeIndices.get(),Buffer::IndexSize::UINT16,0);
    commandBuffer->setViewPort(0,0,multiSampled->width(),multiSampled->height(),1,0);
    commandBuffer->setScissors(slag::Rectangle{.offset = {0,0},.extent = {multiSampled->width(),multiSampled->height()}});
    commandBuffer->drawIndexed(CubeIndices->countAsArray<uint16_t>(),1,0,0,0);

    commandBuffer->endRendering();
    commandBuffer->insertBarrier(
        TextureBarrier
        {
            .texture = multiSampled.get(),
            .accessBefore = BarrierAccessFlags::SHADER_WRITE,
            .accessAfter = BarrierAccessFlags::BLIT_READ,
            .syncBefore = PipelineStageFlags::ALL_GRAPHICS,
            .syncAfter = PipelineStageFlags::BLIT,
        });
    commandBuffer->resolve(multiSampled.get(),0,0,Offset{30,30},output.get(),0,0,Offset{75,75}, Extent{75,75});
    commandBuffer->insertBarrier(
        TextureBarrier
        {
            .texture = output.get(),
            .accessBefore = BarrierAccessFlags::BLIT_WRITE,
            .accessAfter = BarrierAccessFlags::TRANSFER_READ,
            .syncBefore = PipelineStageFlags::BLIT,
            .syncAfter = PipelineStageFlags::TRANSFER,
        });
    TextureBufferMapping copyData
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
         .textureExtent = {output->width(0),output->height(0),1}
    };
    commandBuffer->copyTextureToBuffer(output.get(),outputBuffer.get(),&copyData,1);
    commandBuffer->end();

    CommandBuffer* submitBuffers[1] = {commandBuffer.get()};
    SemaphoreValue signal{.semaphore = finished.get(), .value = 1};
    QueueSubmissionBatch submissionData
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = submitBuffers,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1,
    };
    slagGraphicsCard()->graphicsQueue()->submit(&submissionData,1);
    finished->waitForValue(1);

    unsigned char* pixels = outputBuffer->as<unsigned char>();

    auto groundTruth = utilities::loadTexelsFromFile("resources/textures/resolve-test-result.png");
    GTEST_ASSERT_EQ(outputBuffer->countAsArray<unsigned char>(),groundTruth.size());
    float maxDifference = 255.0f*(1-.95);
    for (auto i=0; i< groundTruth.size(); i+=4)
    {
        float drawnRed = pixels[i];
        float drawnGreen = pixels[i+1];
        float drawnBlue = pixels[i+2];
        float drawnAlpha = pixels[i+3];

        float groundRed = groundTruth[i];
        float groundGreen = groundTruth[i+1];
        float groundBlue = groundTruth[i+2];
        float groundAlpha = groundTruth[i+3];

        float difRed = std::abs(drawnRed-groundRed);
        float difGreen = std::abs(drawnGreen-groundGreen);
        float difBlue = std::abs(drawnBlue-groundBlue);
        float difAlpha = std::abs(drawnAlpha-groundAlpha);

        float pixelDifference = (difRed + difGreen + difBlue + difAlpha)/4;

        GTEST_ASSERT_LE(pixelDifference,maxDifference);
    }
}

TEST_F(CommandBufferTest, FillBuffer)
{
    std::unique_ptr<Buffer> buffer = std::unique_ptr<Buffer>(Buffer::newBuffer(100*sizeof(uint32_t),Buffer::Accessibility::CPU_AND_GPU));
    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::GRAPHICS));
    std::unique_ptr<Semaphore> finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));

    commandBuffer->begin();

    commandBuffer->fillBuffer(buffer.get(),0,buffer->size()/2,15);
    commandBuffer->fillBuffer(buffer.get(),buffer->size()/2,buffer->size()/2,255);

    commandBuffer->end();

    CommandBuffer* submitBuffers[1] = {commandBuffer.get()};
    SemaphoreValue signal{.semaphore = finished.get(), .value = 1};
    QueueSubmissionBatch submissionData
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = submitBuffers,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1,
    };
    slagGraphicsCard()->graphicsQueue()->submit(&submissionData,1);
    finished->waitForValue(1);

    uint32_t* uintPtr = buffer->as<uint32_t>();
    for (auto i = 0; i < buffer->countAsArray<uint32_t>()/2; ++i)
    {
        GTEST_ASSERT_EQ(uintPtr[i],15);
    }
    for (auto i =  buffer->countAsArray<uint32_t>()/2; i < buffer->countAsArray<uint32_t>(); ++i)
    {
        GTEST_ASSERT_EQ(uintPtr[i],255);
    }
}

TEST_F(CommandBufferTest, SetViewport)
{
    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::GRAPHICS));
    std::unique_ptr<Semaphore> finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    std::unique_ptr<Buffer> globalsBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(GlobalSet0Group),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_BUFFER));
    std::unique_ptr<Buffer> objectBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(TexturedDepthSet1Group),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_BUFFER));
    std::unique_ptr<Texture> objectTexture = utilities::loadTextureFromFile("resources/textures/gradient.jpg");
    std::unique_ptr<Texture> target = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::RENDER_TARGET_ATTACHMENT,150,150,1,1,1));
    std::unique_ptr<Texture> depth = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::D24_UNORM_S8_UINT,Texture::Type::TEXTURE_2D,Texture::UsageFlags::DEPTH_STENCIL_ATTACHMENT,150,150,1,1,1));
    std::unique_ptr<Buffer> outputBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(target->byteSize(),Buffer::Accessibility::CPU_AND_GPU));
    std::unique_ptr<DescriptorPool> descriptorPool = std::unique_ptr<DescriptorPool>(DescriptorPool::newDescriptorPool());

    commandBuffer->begin();
    descriptorPool->reset();
    commandBuffer->bindDescriptorPool(descriptorPool.get());
    auto globalBundle = descriptorPool->makeBundle(TexturedDepthPipeline->descriptorGroup(0));
    auto objectBundle = descriptorPool->makeBundle(TexturedDepthPipeline->descriptorGroup(1));
    auto globals = globalsBuffer->as<GlobalSet0Group>();
    auto proj = glm::perspective(95.0f,(float)target->width()/(float)target->height(),.01f,100.0f);
    glm::mat4 view = glm::mat4(1.0f);
    view = glm::translate(view,glm::vec3(0.0f,2.0f,5.0f));
    view = glm::rotate(view,glm::radians(-20.0f),glm::vec3(1.0f,0.0f,0.0f));
    view = glm::inverse(view);//does nothing in this case, but is good practice if we ever do have a camera not at the default location
    glm::mat4 projectionView = proj*view;
    globals->projection = proj;
    globals->view = view;
    globals->projectionView = projectionView;
    auto object = objectBuffer->as<TexturedDepthSet1Group>();
    object->position = glm::rotate(glm::mat4(1.0f),glm::radians(45.0f),glm::vec3(0.0f,1.0f,0.0f));
    globalBundle.setUniformBuffer(0,0,globalsBuffer.get(),0,sizeof(GlobalSet0Group));
    commandBuffer->bindGraphicsShaderPipeline(TexturedDepthPipeline.get());
    commandBuffer->bindGraphicsDescriptorBundle(0,globalBundle);
    objectBundle.setUniformBuffer(0,0,objectBuffer.get(),0,sizeof(TexturedDepthSet1Group));
    objectBundle.setSampledTexture(1,0,objectTexture.get());
    objectBundle.setSampler(2,0,DefaultSampler.get());
    commandBuffer->bindGraphicsDescriptorBundle(0,globalBundle);
    commandBuffer->bindGraphicsDescriptorBundle(1,objectBundle);
    Attachment colorAttachment{.texture = target.get(),.autoClear = true,.clearValue = ClearValue{.color = {.floats = {0,0,0,1}}}};
    Attachment depthAttachment{.texture = depth.get(),.autoClear = true,.clearValue = ClearValue{.depthStencil = {.depth = 1, .stencil = 0}}};
    commandBuffer->beginRendering(&colorAttachment,1,&depthAttachment,slag::Rectangle{.extent = {target->width(),target->height()}});

    Buffer* vertexBuffers[]
    {
        TriangleVerts.get(),
        TriangleUVs.get()
    };
    uint64_t vertexOffsets[]{0,0};
    uint64_t bufferStrides[2] = {sizeof(glm::vec3),sizeof(glm::vec2)};
    commandBuffer->bindVertexBuffers(0,vertexBuffers,vertexOffsets,bufferStrides,2);
    commandBuffer->setViewPort(target->width()/5,target->width()/5,target->width()/2,target->height()/2,0,1);
    commandBuffer->setScissors(slag::Rectangle{.offset = {0,0},.extent = {target->width(),target->height()}});
    commandBuffer->draw(3,1,0,0);

    commandBuffer->endRendering();



    commandBuffer->insertBarrier(TextureBarrier{.texture = target.get(), .accessBefore = BarrierAccessFlags::COLOR_ATTACHMENT_WRITE,.accessAfter = BarrierAccessFlags::TRANSFER_READ,.syncBefore = PipelineStageFlags::ALL_GRAPHICS, .syncAfter = PipelineStageFlags::TRANSFER});
    TextureBufferMapping copyData
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
         .textureExtent = {target->width(0),target->height(0),1}
    };
    commandBuffer->copyTextureToBuffer(target.get(),outputBuffer.get(),&copyData,1);

    commandBuffer->end();

    CommandBuffer* submitBuffers[1] = {commandBuffer.get()};
    SemaphoreValue signal{.semaphore = finished.get(), .value = 1};
    QueueSubmissionBatch submissionData
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = submitBuffers,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1,
    };
    slagGraphicsCard()->graphicsQueue()->submit(&submissionData,1);
    finished->waitForValue(1);

    unsigned char* colorPtr = outputBuffer->as<unsigned char>();

    auto groundTruth = utilities::loadTexelsFromFile("resources/textures/set-viewport-test-result.png");
    GTEST_ASSERT_EQ(outputBuffer->countAsArray<unsigned char>(),groundTruth.size());

    for (auto i=0; i< outputBuffer->countAsArray<unsigned char>(); i++)
    {
        GTEST_ASSERT_EQ(colorPtr[i],groundTruth[i]);
    }

}

TEST_F(CommandBufferTest, SetScissor)
{
    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::GRAPHICS));
    std::unique_ptr<Semaphore> finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    std::unique_ptr<Buffer> globalsBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(GlobalSet0Group),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_BUFFER));
    std::unique_ptr<Buffer> objectBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(TexturedDepthSet1Group),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_BUFFER));
    std::unique_ptr<Texture> objectTexture = utilities::loadTextureFromFile("resources/textures/gradient.jpg");
    std::unique_ptr<Texture> target = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::RENDER_TARGET_ATTACHMENT,150,150,1,1,1));
    std::unique_ptr<Texture> depth = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::D24_UNORM_S8_UINT,Texture::Type::TEXTURE_2D,Texture::UsageFlags::DEPTH_STENCIL_ATTACHMENT,150,150,1,1,1));
    std::unique_ptr<Buffer> outputBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(target->byteSize(),Buffer::Accessibility::CPU_AND_GPU));
    std::unique_ptr<DescriptorPool> descriptorPool = std::unique_ptr<DescriptorPool>(DescriptorPool::newDescriptorPool());

    commandBuffer->begin();
    descriptorPool->reset();
    commandBuffer->bindDescriptorPool(descriptorPool.get());
    auto globalBundle = descriptorPool->makeBundle(TexturedDepthPipeline->descriptorGroup(0));
    auto objectBundle = descriptorPool->makeBundle(TexturedDepthPipeline->descriptorGroup(1));
    auto globals = globalsBuffer->as<GlobalSet0Group>();
    auto proj = glm::perspective(95.0f,(float)target->width()/(float)target->height(),.01f,100.0f);
    glm::mat4 view = glm::mat4(1.0f);
    view = glm::translate(view,glm::vec3(0.0f,2.0f,5.0f));
    view = glm::rotate(view,glm::radians(-20.0f),glm::vec3(1.0f,0.0f,0.0f));
    view = glm::inverse(view);//does nothing in this case, but is good practice if we ever do have a camera not at the default location
    glm::mat4 projectionView = proj*view;
    globals->projection = proj;
    globals->view = view;
    globals->projectionView = projectionView;
    auto object = objectBuffer->as<TexturedDepthSet1Group>();
    object->position = glm::rotate(glm::mat4(1.0f),glm::radians(45.0f),glm::vec3(0.0f,1.0f,0.0f));
    globalBundle.setUniformBuffer(0,0,globalsBuffer.get(),0,sizeof(GlobalSet0Group));
    commandBuffer->bindGraphicsShaderPipeline(TexturedDepthPipeline.get());
    commandBuffer->bindGraphicsDescriptorBundle(0,globalBundle);
    objectBundle.setUniformBuffer(0,0,objectBuffer.get(),0,sizeof(TexturedDepthSet1Group));
    objectBundle.setSampledTexture(1,0,objectTexture.get());
    objectBundle.setSampler(2,0,DefaultSampler.get());
    commandBuffer->bindGraphicsDescriptorBundle(0,globalBundle);
    commandBuffer->bindGraphicsDescriptorBundle(1,objectBundle);
    Attachment colorAttachment{.texture = target.get(),.autoClear = true,.clearValue = ClearValue{.color = {.floats = {0,0,0,1}}}};
    Attachment depthAttachment{.texture = depth.get(),.autoClear = true,.clearValue = ClearValue{.depthStencil = {.depth = 1, .stencil = 0}}};
    commandBuffer->beginRendering(&colorAttachment,1,&depthAttachment,slag::Rectangle{.extent = {target->width(),target->height()}});

    Buffer* vertexBuffers[]
    {
        TriangleVerts.get(),
        TriangleUVs.get()
    };
    uint64_t vertexOffsets[]{0,0};
    uint64_t bufferStrides[2] = {sizeof(glm::vec3),sizeof(glm::vec2)};
    commandBuffer->bindVertexBuffers(0,vertexBuffers,vertexOffsets,bufferStrides,2);
    commandBuffer->setViewPort(0,0,target->width(),target->height(),0,1);
    commandBuffer->setScissors(slag::Rectangle{.offset = {60,60},.extent = {target->width()/4,target->height()/4}});
    commandBuffer->draw(3,1,0,0);

    commandBuffer->endRendering();



    commandBuffer->insertBarrier(TextureBarrier{.texture = target.get(), .accessBefore = BarrierAccessFlags::COLOR_ATTACHMENT_WRITE,.accessAfter = BarrierAccessFlags::TRANSFER_READ,.syncBefore = PipelineStageFlags::ALL_GRAPHICS, .syncAfter = PipelineStageFlags::TRANSFER});
    TextureBufferMapping copyData
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
         .textureExtent = {target->width(0),target->height(0),1}
    };
    commandBuffer->copyTextureToBuffer(target.get(),outputBuffer.get(),&copyData,1);

    commandBuffer->end();

    CommandBuffer* submitBuffers[1] = {commandBuffer.get()};
    SemaphoreValue signal{.semaphore = finished.get(), .value = 1};
    QueueSubmissionBatch submissionData
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = submitBuffers,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1,
    };
    slagGraphicsCard()->graphicsQueue()->submit(&submissionData,1);
    finished->waitForValue(1);

    unsigned char* colorPtr = outputBuffer->as<unsigned char>();

    auto groundTruth = utilities::loadTexelsFromFile("resources/textures/set-scissor-test-result.png");
    GTEST_ASSERT_EQ(outputBuffer->countAsArray<unsigned char>(),groundTruth.size());

    for (auto i=0; i< outputBuffer->countAsArray<unsigned char>(); i++)
    {
        GTEST_ASSERT_EQ(colorPtr[i],groundTruth[i]);
    }
}

TEST_F(CommandBufferTest, SetBlendConstants)
{
    //Skip, as we don't have a great way to test this
    GTEST_SKIP();
}

TEST_F(CommandBufferTest, SetStencilReference)
{
    //Skip, as we don't have a great way to test this
    GTEST_SKIP();
}

TEST_F(CommandBufferTest, Draw)
{
    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::GRAPHICS));
    std::unique_ptr<Semaphore> finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    std::unique_ptr<Buffer> globalsBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(GlobalSet0Group),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_BUFFER));
    std::unique_ptr<Buffer> objectBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(TexturedDepthSet1Group),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_BUFFER));
    std::unique_ptr<Texture> objectTexture = utilities::loadTextureFromFile("resources/textures/gradient.jpg");
    std::unique_ptr<Texture> target = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::RENDER_TARGET_ATTACHMENT,150,150,1,1,1));
    std::unique_ptr<Texture> depth = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::D24_UNORM_S8_UINT,Texture::Type::TEXTURE_2D,Texture::UsageFlags::DEPTH_STENCIL_ATTACHMENT,150,150,1,1,1));
    std::unique_ptr<Buffer> outputBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(target->byteSize(),Buffer::Accessibility::CPU_AND_GPU));
    std::unique_ptr<DescriptorPool> descriptorPool = std::unique_ptr<DescriptorPool>(DescriptorPool::newDescriptorPool());

    commandBuffer->begin();
    descriptorPool->reset();
    commandBuffer->bindDescriptorPool(descriptorPool.get());
    auto globalBundle = descriptorPool->makeBundle(TexturedDepthPipeline->descriptorGroup(0));
    auto objectBundle = descriptorPool->makeBundle(TexturedDepthPipeline->descriptorGroup(1));
    auto globals = globalsBuffer->as<GlobalSet0Group>();
    auto proj = glm::perspective(95.0f,(float)target->width()/(float)target->height(),.01f,100.0f);
    glm::mat4 view = glm::mat4(1.0f);
    view = glm::translate(view,glm::vec3(0.0f,2.0f,5.0f));
    view = glm::rotate(view,glm::radians(-20.0f),glm::vec3(1.0f,0.0f,0.0f));
    view = glm::inverse(view);//does nothing in this case, but is good practice if we ever do have a camera not at the default location
    glm::mat4 projectionView = proj*view;
    globals->projection = proj;
    globals->view = view;
    globals->projectionView = projectionView;
    auto object = objectBuffer->as<TexturedDepthSet1Group>();
    object->position = glm::rotate(glm::mat4(1.0f),glm::radians(45.0f),glm::vec3(0.0f,1.0f,0.0f));
    globalBundle.setUniformBuffer(0,0,globalsBuffer.get(),0,sizeof(GlobalSet0Group));
    commandBuffer->bindGraphicsShaderPipeline(TexturedDepthPipeline.get());
    commandBuffer->bindGraphicsDescriptorBundle(0,globalBundle);
    objectBundle.setUniformBuffer(0,0,objectBuffer.get(),0,sizeof(TexturedDepthSet1Group));
    objectBundle.setSampledTexture(1,0,objectTexture.get());
    objectBundle.setSampler(2,0,DefaultSampler.get());
    commandBuffer->bindGraphicsDescriptorBundle(0,globalBundle);
    commandBuffer->bindGraphicsDescriptorBundle(1,objectBundle);
    Attachment colorAttachment{.texture = target.get(),.autoClear = true,.clearValue = ClearValue{.color = {.floats = {0,0,0,1}}}};
    Attachment depthAttachment{.texture = depth.get(),.autoClear = true,.clearValue = ClearValue{.depthStencil = {.depth = 1, .stencil = 0}}};
    commandBuffer->beginRendering(&colorAttachment,1,&depthAttachment,slag::Rectangle{.extent = {target->width(),target->height()}});

    Buffer* vertexBuffers[]
    {
        TriangleVerts.get(),
        TriangleUVs.get()
    };
    uint64_t vertexOffsets[]{0,0};
    uint64_t bufferStrides[2] = {sizeof(glm::vec3),sizeof(glm::vec2)};
    commandBuffer->bindVertexBuffers(0,vertexBuffers,vertexOffsets,bufferStrides,2);
    commandBuffer->setViewPort(0,0,target->width(),target->height(),1,0);
    commandBuffer->setScissors(slag::Rectangle{.offset = {0,0},.extent = {target->width(),target->height()}});
    commandBuffer->draw(3,1,0,0);

    commandBuffer->endRendering();



    commandBuffer->insertBarrier(TextureBarrier{.texture = target.get(), .accessBefore = BarrierAccessFlags::COLOR_ATTACHMENT_WRITE,.accessAfter = BarrierAccessFlags::TRANSFER_READ,.syncBefore = PipelineStageFlags::ALL_GRAPHICS, .syncAfter = PipelineStageFlags::TRANSFER});
    TextureBufferMapping copyData
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
         .textureExtent = {target->width(0),target->height(0),1}
    };
    commandBuffer->copyTextureToBuffer(target.get(),outputBuffer.get(),&copyData,1);

    commandBuffer->end();

    CommandBuffer* submitBuffers[1] = {commandBuffer.get()};
    SemaphoreValue signal{.semaphore = finished.get(), .value = 1};
    QueueSubmissionBatch submissionData
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = submitBuffers,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1,
    };
    slagGraphicsCard()->graphicsQueue()->submit(&submissionData,1);
    finished->waitForValue(1);

    unsigned char* colorPtr = outputBuffer->as<unsigned char>();

    auto groundTruth = utilities::loadTexelsFromFile("resources/textures/draw-triangle-test-result.png");
    GTEST_ASSERT_EQ(outputBuffer->countAsArray<unsigned char>(),groundTruth.size());

    for (auto i=0; i< outputBuffer->countAsArray<unsigned char>(); i++)
    {
        GTEST_ASSERT_EQ(colorPtr[i],groundTruth[i]);
    }

}

TEST_F(CommandBufferTest, DrawIndexed)
{
    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::GRAPHICS));
    std::unique_ptr<Semaphore> finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    std::unique_ptr<Buffer> globalsBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(GlobalSet0Group),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_BUFFER));
    std::unique_ptr<Buffer> objectBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(TexturedDepthSet1Group),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_BUFFER));
    std::unique_ptr<Texture> objectTexture = utilities::loadTextureFromFile("resources/textures/gradient.jpg");
    std::unique_ptr<Texture> target = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::RENDER_TARGET_ATTACHMENT,150,150,1,1,1));
    std::unique_ptr<Texture> depth = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::D24_UNORM_S8_UINT,Texture::Type::TEXTURE_2D,Texture::UsageFlags::DEPTH_STENCIL_ATTACHMENT,150,150,1,1,1));
    std::unique_ptr<Buffer> outputBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(target->byteSize(),Buffer::Accessibility::CPU_AND_GPU));
    std::unique_ptr<DescriptorPool> descriptorPool = std::unique_ptr<DescriptorPool>(DescriptorPool::newDescriptorPool());

    commandBuffer->begin();
    descriptorPool->reset();
    commandBuffer->bindDescriptorPool(descriptorPool.get());
    auto globalBundle = descriptorPool->makeBundle(TexturedDepthPipeline->descriptorGroup(0));
    auto objectBundle = descriptorPool->makeBundle(TexturedDepthPipeline->descriptorGroup(1));
    auto globals = globalsBuffer->as<GlobalSet0Group>();
    auto proj = glm::perspective(95.0f,(float)target->width()/(float)target->height(),.01f,100.0f);
    glm::mat4 view = glm::mat4(1.0f);
    view = glm::translate(view,glm::vec3(0.0f,2.0f,5.0f));
    view = glm::rotate(view,glm::radians(-20.0f),glm::vec3(1.0f,0.0f,0.0f));
    view = glm::inverse(view);//does nothing in this case, but is good practice if we ever do have a camera not at the default location
    glm::mat4 projectionView = proj*view;
    globals->projection = proj;
    globals->view = view;
    globals->projectionView = projectionView;
    auto object = objectBuffer->as<TexturedDepthSet1Group>();
    object->position = glm::rotate(glm::mat4(1.0f),glm::radians(45.0f),glm::vec3(0.0f,1.0f,0.0f));
    globalBundle.setUniformBuffer(0,0,globalsBuffer.get(),0,sizeof(GlobalSet0Group));
    commandBuffer->bindGraphicsShaderPipeline(TexturedDepthPipeline.get());
    commandBuffer->bindGraphicsDescriptorBundle(0,globalBundle);
    objectBundle.setUniformBuffer(0,0,objectBuffer.get(),0,sizeof(TexturedDepthSet1Group));
    objectBundle.setSampledTexture(1,0,objectTexture.get());
    objectBundle.setSampler(2,0,DefaultSampler.get());
    commandBuffer->bindGraphicsDescriptorBundle(0,globalBundle);
    commandBuffer->bindGraphicsDescriptorBundle(1,objectBundle);
    Attachment colorAttachment{.texture = target.get(),.autoClear = true,.clearValue = ClearValue{.color = {.floats = {0,0,0,1}}}};
    Attachment depthAttachment{.texture = depth.get(),.autoClear = true,.clearValue = ClearValue{.depthStencil = {.depth = 1, .stencil = 0}}};
    commandBuffer->beginRendering(&colorAttachment,1,&depthAttachment,slag::Rectangle{.extent = {target->width(),target->height()}});

    Buffer* vertexBuffers[]
    {
        CubeVerts.get(),
        CubeUVs.get()
    };
    uint64_t vertexOffsets[]{0,0};
    uint64_t bufferStrides[2] = {sizeof(glm::vec3),sizeof(glm::vec2)};
    commandBuffer->bindVertexBuffers(0,vertexBuffers,vertexOffsets,bufferStrides,2);
    commandBuffer->bindIndexBuffer(CubeIndices.get(),Buffer::IndexSize::UINT16,0);
    commandBuffer->setViewPort(0,0,target->width(),target->height(),1,0);
    commandBuffer->setScissors(slag::Rectangle{.offset = {0,0},.extent = {target->width(),target->height()}});
    commandBuffer->drawIndexed(CubeIndices->countAsArray<uint16_t>(),1,0,0,0);

    commandBuffer->endRendering();

    commandBuffer->insertBarrier(TextureBarrier{.texture = target.get(), .accessBefore = BarrierAccessFlags::COLOR_ATTACHMENT_WRITE,.accessAfter = BarrierAccessFlags::TRANSFER_READ,.syncBefore = PipelineStageFlags::ALL_GRAPHICS, .syncAfter = PipelineStageFlags::TRANSFER});
    TextureBufferMapping copyData
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
         .textureExtent = {target->width(0),target->height(0),1}
    };
    commandBuffer->copyTextureToBuffer(target.get(),outputBuffer.get(),&copyData,1);

    commandBuffer->end();

    CommandBuffer* submitBuffers[1] = {commandBuffer.get()};
    SemaphoreValue signal{.semaphore = finished.get(), .value = 1};
    QueueSubmissionBatch submissionData
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = submitBuffers,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1,
    };
    slagGraphicsCard()->graphicsQueue()->submit(&submissionData,1);
    finished->waitForValue(1);

    unsigned char* colorPtr = outputBuffer->as<unsigned char>();
    auto groundTruth = utilities::loadTexelsFromFile("resources/textures/draw-test-result.png");
    GTEST_ASSERT_EQ(outputBuffer->countAsArray<unsigned char>(),groundTruth.size());

    for (auto i=0; i< outputBuffer->countAsArray<unsigned char>(); i++)
    {
        GTEST_ASSERT_EQ(colorPtr[i],groundTruth[i]);
    }
}

TEST_F(CommandBufferTest, DrawIndexedIndirect)
{
    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::GRAPHICS));
    std::unique_ptr<Semaphore> finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    std::unique_ptr<Buffer> globalsBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(GlobalSet0Group),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_BUFFER));
    std::unique_ptr<Buffer> objectBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(TexturedDepthSet1Group),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_BUFFER));
    std::unique_ptr<Texture> objectTexture = utilities::loadTextureFromFile("resources/textures/gradient.jpg");
    std::unique_ptr<Texture> target = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::RENDER_TARGET_ATTACHMENT,150,150,1,1,1));
    std::unique_ptr<Texture> depth = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::D24_UNORM_S8_UINT,Texture::Type::TEXTURE_2D,Texture::UsageFlags::DEPTH_STENCIL_ATTACHMENT,150,150,1,1,1));
    std::unique_ptr<Buffer> outputBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(target->byteSize(),Buffer::Accessibility::CPU_AND_GPU));
    std::unique_ptr<DescriptorPool> descriptorPool = std::unique_ptr<DescriptorPool>(DescriptorPool::newDescriptorPool());

    commandBuffer->begin();
    descriptorPool->reset();
    commandBuffer->bindDescriptorPool(descriptorPool.get());
    auto globalBundle = descriptorPool->makeBundle(TexturedDepthPipeline->descriptorGroup(0));
    auto objectBundle = descriptorPool->makeBundle(TexturedDepthPipeline->descriptorGroup(1));
    auto globals = globalsBuffer->as<GlobalSet0Group>();
    auto proj = glm::perspective(95.0f,(float)target->width()/(float)target->height(),.01f,100.0f);
    glm::mat4 view = glm::mat4(1.0f);
    view = glm::translate(view,glm::vec3(0.0f,2.0f,5.0f));
    view = glm::rotate(view,glm::radians(-20.0f),glm::vec3(1.0f,0.0f,0.0f));
    view = glm::inverse(view);//does nothing in this case, but is good practice if we ever do have a camera not at the default location
    glm::mat4 projectionView = proj*view;
    globals->projection = proj;
    globals->view = view;
    globals->projectionView = projectionView;
    auto object = objectBuffer->as<TexturedDepthSet1Group>();
    object->position = glm::rotate(glm::mat4(1.0f),glm::radians(45.0f),glm::vec3(0.0f,1.0f,0.0f));
    globalBundle.setUniformBuffer(0,0,globalsBuffer.get(),0,sizeof(GlobalSet0Group));
    commandBuffer->bindGraphicsShaderPipeline(TexturedDepthPipeline.get());
    commandBuffer->bindGraphicsDescriptorBundle(0,globalBundle);
    objectBundle.setUniformBuffer(0,0,objectBuffer.get(),0,sizeof(TexturedDepthSet1Group));
    objectBundle.setSampledTexture(1,0,objectTexture.get());
    objectBundle.setSampler(2,0,DefaultSampler.get());
    commandBuffer->bindGraphicsDescriptorBundle(0,globalBundle);
    commandBuffer->bindGraphicsDescriptorBundle(1,objectBundle);
    Attachment colorAttachment{.texture = target.get(),.autoClear = true,.clearValue = ClearValue{.color = {.floats = {0,0,0,1}}}};
    Attachment depthAttachment{.texture = depth.get(),.autoClear = true,.clearValue = ClearValue{.depthStencil = {.depth = 1, .stencil = 0}}};
    commandBuffer->beginRendering(&colorAttachment,1,&depthAttachment,slag::Rectangle{.extent = {target->width(),target->height()}});

    Buffer* vertexBuffers[]
    {
        CubeVerts.get(),
        CubeUVs.get()
    };
    uint64_t vertexOffsets[]{0,0};
    uint64_t bufferStrides[2] = {sizeof(glm::vec3),sizeof(glm::vec2)};
    commandBuffer->bindVertexBuffers(0,vertexBuffers,vertexOffsets,bufferStrides,2);
    commandBuffer->bindIndexBuffer(CubeIndices.get(),Buffer::IndexSize::UINT16,0);
    std::unique_ptr<Buffer> drawParams = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(IndirectDrawIndexedCommand),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::INDIRECT_BUFFER));
    auto drawParamPtr = drawParams->as<IndirectDrawIndexedCommand>();

    drawParamPtr->indexCount = CubeIndices->countAsArray<uint16_t>();
    drawParamPtr->instanceCount = 1;
    drawParamPtr->firstIndex = 0;
    drawParamPtr->vertexOffset = 0;
    drawParamPtr->firstInstance = 0;

    commandBuffer->setViewPort(0,0,target->width(),target->height(),1,0);
    commandBuffer->setScissors(slag::Rectangle{.offset = {0,0},.extent = {target->width(),target->height()}});
    commandBuffer->drawIndexedIndirect(drawParams.get(),0,1,sizeof(IndirectDrawIndexedCommand));

    commandBuffer->endRendering();

    commandBuffer->insertBarrier(TextureBarrier{.texture = target.get(), .accessBefore = BarrierAccessFlags::COLOR_ATTACHMENT_WRITE,.accessAfter = BarrierAccessFlags::TRANSFER_READ,.syncBefore = PipelineStageFlags::ALL_GRAPHICS, .syncAfter = PipelineStageFlags::TRANSFER});
    TextureBufferMapping copyData
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
         .textureExtent = {target->width(0),target->height(0),1}
    };
    commandBuffer->copyTextureToBuffer(target.get(),outputBuffer.get(),&copyData,1);
    commandBuffer->end();

    CommandBuffer* submitBuffers[1] = {commandBuffer.get()};
    SemaphoreValue signal{.semaphore = finished.get(), .value = 1};
    QueueSubmissionBatch submissionData
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = submitBuffers,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1,
    };
    slagGraphicsCard()->graphicsQueue()->submit(&submissionData,1);
    finished->waitForValue(1);

    unsigned char* colorPtr = outputBuffer->as<unsigned char>();
    auto groundTruth = utilities::loadTexelsFromFile("resources/textures/draw-test-result.png");
    GTEST_ASSERT_EQ(outputBuffer->countAsArray<unsigned char>(),groundTruth.size());

    for (auto i=0; i< outputBuffer->countAsArray<unsigned char>(); i++)
    {
        GTEST_ASSERT_EQ(colorPtr[i],groundTruth[i]);
    }
}

TEST_F(CommandBufferTest, DrawIndexedIndirectCount)
{
    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::GRAPHICS));
    std::unique_ptr<Semaphore> finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    std::unique_ptr<Buffer> globalsBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(GlobalSet0Group),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_BUFFER));
    std::unique_ptr<Buffer> objectBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(TexturedDepthSet1Group),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_BUFFER));
    std::unique_ptr<Texture> objectTexture = utilities::loadTextureFromFile("resources/textures/gradient.jpg");
    std::unique_ptr<Texture> target = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::RENDER_TARGET_ATTACHMENT,150,150,1,1,1));
    std::unique_ptr<Texture> depth = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::D24_UNORM_S8_UINT,Texture::Type::TEXTURE_2D,Texture::UsageFlags::DEPTH_STENCIL_ATTACHMENT,150,150,1,1,1));
    std::unique_ptr<Buffer> outputBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(target->byteSize(),Buffer::Accessibility::CPU_AND_GPU));
    std::unique_ptr<DescriptorPool> descriptorPool = std::unique_ptr<DescriptorPool>(DescriptorPool::newDescriptorPool());

    commandBuffer->begin();
    descriptorPool->reset();
    commandBuffer->bindDescriptorPool(descriptorPool.get());
    auto globalBundle = descriptorPool->makeBundle(TexturedDepthPipeline->descriptorGroup(0));
    auto objectBundle = descriptorPool->makeBundle(TexturedDepthPipeline->descriptorGroup(1));
    auto globals = globalsBuffer->as<GlobalSet0Group>();
    auto proj = glm::perspective(95.0f,(float)target->width()/(float)target->height(),.01f,100.0f);
    glm::mat4 view = glm::mat4(1.0f);
    view = glm::translate(view,glm::vec3(0.0f,2.0f,5.0f));
    view = glm::rotate(view,glm::radians(-20.0f),glm::vec3(1.0f,0.0f,0.0f));
    view = glm::inverse(view);//does nothing in this case, but is good practice if we ever do have a camera not at the default location
    glm::mat4 projectionView = proj*view;
    globals->projection = proj;
    globals->view = view;
    globals->projectionView = projectionView;
    auto object = objectBuffer->as<TexturedDepthSet1Group>();
    object->position = glm::rotate(glm::mat4(1.0f),glm::radians(45.0f),glm::vec3(0.0f,1.0f,0.0f));
    globalBundle.setUniformBuffer(0,0,globalsBuffer.get(),0,sizeof(GlobalSet0Group));
    commandBuffer->bindGraphicsShaderPipeline(TexturedDepthPipeline.get());
    commandBuffer->bindGraphicsDescriptorBundle(0,globalBundle);
    objectBundle.setUniformBuffer(0,0,objectBuffer.get(),0,sizeof(TexturedDepthSet1Group));
    objectBundle.setSampledTexture(1,0,objectTexture.get());
    objectBundle.setSampler(2,0,DefaultSampler.get());
    commandBuffer->bindGraphicsDescriptorBundle(0,globalBundle);
    commandBuffer->bindGraphicsDescriptorBundle(1,objectBundle);
    Attachment colorAttachment{.texture = target.get(),.autoClear = true,.clearValue = ClearValue{.color = {.floats = {0,0,0,1}}}};
    Attachment depthAttachment{.texture = depth.get(),.autoClear = true,.clearValue = ClearValue{.depthStencil = {.depth = 1, .stencil = 0}}};
    commandBuffer->beginRendering(&colorAttachment,1,&depthAttachment,slag::Rectangle{.extent = {target->width(),target->height()}});

    Buffer* vertexBuffers[]
    {
        CubeVerts.get(),
        CubeUVs.get()
    };
    uint64_t vertexOffsets[]{0,0};
    uint64_t bufferStrides[2] = {sizeof(glm::vec3),sizeof(glm::vec2)};
    commandBuffer->bindVertexBuffers(0,vertexBuffers,vertexOffsets,bufferStrides,2);
    commandBuffer->bindIndexBuffer(CubeIndices.get(),Buffer::IndexSize::UINT16,0);
    std::unique_ptr<Buffer> drawParams = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(IndirectDrawIndexedCommand),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::INDIRECT_BUFFER));
    auto drawParamPtr = drawParams->as<IndirectDrawIndexedCommand>();

    drawParamPtr->indexCount = CubeIndices->countAsArray<uint16_t>();
    drawParamPtr->instanceCount = 1;
    drawParamPtr->firstIndex = 0;
    drawParamPtr->vertexOffset = 0;
    drawParamPtr->firstInstance = 0;

    uint32_t drawCount = 1;
    std::unique_ptr<Buffer> drawCountParams = std::unique_ptr<Buffer>(Buffer::newBuffer(&drawCount,sizeof(uint32_t),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::INDIRECT_BUFFER));
    commandBuffer->setViewPort(0,0,target->width(),target->height(),1,0);
    commandBuffer->setScissors(slag::Rectangle{.offset = {0,0},.extent = {target->width(),target->height()}});
    commandBuffer->drawIndexedIndirectCount(drawParams.get(),0,drawCountParams.get(),0,1,sizeof(IndirectDrawIndexedCommand));

    commandBuffer->endRendering();

    commandBuffer->insertBarrier(TextureBarrier{.texture = target.get(), .accessBefore = BarrierAccessFlags::COLOR_ATTACHMENT_WRITE,.accessAfter = BarrierAccessFlags::TRANSFER_READ,.syncBefore = PipelineStageFlags::ALL_GRAPHICS, .syncAfter = PipelineStageFlags::TRANSFER});
    TextureBufferMapping copyData
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
         .textureExtent = {target->width(0),target->height(0),1}
    };
    commandBuffer->copyTextureToBuffer(target.get(),outputBuffer.get(),&copyData,1);

    commandBuffer->end();

    CommandBuffer* submitBuffers[1] = {commandBuffer.get()};
    SemaphoreValue signal{.semaphore = finished.get(), .value = 1};
    QueueSubmissionBatch submissionData
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = submitBuffers,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1,
    };
    slagGraphicsCard()->graphicsQueue()->submit(&submissionData,1);
    finished->waitForValue(1);

    unsigned char* colorPtr = outputBuffer->as<unsigned char>();
    auto groundTruth = utilities::loadTexelsFromFile("resources/textures/draw-test-result.png");
    GTEST_ASSERT_EQ(outputBuffer->countAsArray<unsigned char>(),groundTruth.size());

    for (auto i=0; i< outputBuffer->countAsArray<unsigned char>(); i++)
    {
        GTEST_ASSERT_EQ(colorPtr[i],groundTruth[i]);
    }
}

TEST_F(CommandBufferTest, DrawIndirect)
{
    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::GRAPHICS));
    std::unique_ptr<Semaphore> finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    std::unique_ptr<Buffer> globalsBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(GlobalSet0Group),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_BUFFER));
    std::unique_ptr<Buffer> objectBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(TexturedDepthSet1Group),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_BUFFER));
    std::unique_ptr<Texture> objectTexture = utilities::loadTextureFromFile("resources/textures/gradient.jpg");
    std::unique_ptr<Texture> target = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::RENDER_TARGET_ATTACHMENT,150,150,1,1,1));
    std::unique_ptr<Texture> depth = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::D24_UNORM_S8_UINT,Texture::Type::TEXTURE_2D,Texture::UsageFlags::DEPTH_STENCIL_ATTACHMENT,150,150,1,1,1));
    std::unique_ptr<Buffer> outputBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(target->byteSize(),Buffer::Accessibility::CPU_AND_GPU));
    std::unique_ptr<DescriptorPool> descriptorPool = std::unique_ptr<DescriptorPool>(DescriptorPool::newDescriptorPool());

    commandBuffer->begin();
    descriptorPool->reset();
    commandBuffer->bindDescriptorPool(descriptorPool.get());
    auto globalBundle = descriptorPool->makeBundle(TexturedDepthPipeline->descriptorGroup(0));
    auto objectBundle = descriptorPool->makeBundle(TexturedDepthPipeline->descriptorGroup(1));
    auto globals = globalsBuffer->as<GlobalSet0Group>();
    auto proj = glm::perspective(95.0f,(float)target->width()/(float)target->height(),.01f,100.0f);
    glm::mat4 view = glm::mat4(1.0f);
    view = glm::translate(view,glm::vec3(0.0f,2.0f,5.0f));
    view = glm::rotate(view,glm::radians(-20.0f),glm::vec3(1.0f,0.0f,0.0f));
    view = glm::inverse(view);//does nothing in this case, but is good practice if we ever do have a camera not at the default location
    glm::mat4 projectionView = proj*view;
    globals->projection = proj;
    globals->view = view;
    globals->projectionView = projectionView;
    auto object = objectBuffer->as<TexturedDepthSet1Group>();
    object->position = glm::rotate(glm::mat4(1.0f),glm::radians(45.0f),glm::vec3(0.0f,1.0f,0.0f));
    globalBundle.setUniformBuffer(0,0,globalsBuffer.get(),0,sizeof(GlobalSet0Group));
    commandBuffer->bindGraphicsShaderPipeline(TexturedDepthPipeline.get());
    commandBuffer->bindGraphicsDescriptorBundle(0,globalBundle);
    objectBundle.setUniformBuffer(0,0,objectBuffer.get(),0,sizeof(TexturedDepthSet1Group));
    objectBundle.setSampledTexture(1,0,objectTexture.get());
    objectBundle.setSampler(2,0,DefaultSampler.get());
    commandBuffer->bindGraphicsDescriptorBundle(0,globalBundle);
    commandBuffer->bindGraphicsDescriptorBundle(1,objectBundle);
    Attachment colorAttachment{.texture = target.get(),.autoClear = true,.clearValue = ClearValue{.color = {.floats = {0,0,0,1}}}};
    Attachment depthAttachment{.texture = depth.get(),.autoClear = true,.clearValue = ClearValue{.depthStencil = {.depth = 1, .stencil = 0}}};
    commandBuffer->beginRendering(&colorAttachment,1,&depthAttachment,slag::Rectangle{.extent = {target->width(),target->height()}});

    Buffer* vertexBuffers[]
    {
        TriangleVerts.get(),
        TriangleUVs.get()
    };
    uint64_t vertexOffsets[]{0,0};
    uint64_t bufferStrides[2] = {sizeof(glm::vec3),sizeof(glm::vec2)};
    commandBuffer->bindVertexBuffers(0,vertexBuffers,vertexOffsets,bufferStrides,2);

    std::unique_ptr<Buffer> drawCommands = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(IndirectDrawCommand),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::INDIRECT_BUFFER));
    auto drawCommandPtr = drawCommands->as<IndirectDrawCommand>();
    drawCommandPtr->vertexCount = 3;
    drawCommandPtr->instanceCount = 1;
    drawCommandPtr->firstVertex = 0;
    drawCommandPtr->firstInstance = 0;
    commandBuffer->setViewPort(0,0,target->width(),target->height(),1,0);
    commandBuffer->setScissors(slag::Rectangle{.offset = {0,0},.extent = {target->width(),target->height()}});
    commandBuffer->drawIndirect(drawCommands.get(),0,1,sizeof(IndirectDrawCommand));
    commandBuffer->endRendering();
    commandBuffer->insertBarrier(TextureBarrier{.texture = target.get(), .accessBefore = BarrierAccessFlags::COLOR_ATTACHMENT_WRITE,.accessAfter = BarrierAccessFlags::TRANSFER_READ,.syncBefore = PipelineStageFlags::ALL_GRAPHICS, .syncAfter = PipelineStageFlags::TRANSFER});
    TextureBufferMapping copyData
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
         .textureExtent = {target->width(0),target->height(0),1}
    };
    commandBuffer->copyTextureToBuffer(target.get(),outputBuffer.get(),&copyData,1);


    commandBuffer->end();

    CommandBuffer* submitBuffers[1] = {commandBuffer.get()};
    SemaphoreValue signal{.semaphore = finished.get(), .value = 1};
    QueueSubmissionBatch submissionData
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = submitBuffers,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1,
    };
    slagGraphicsCard()->graphicsQueue()->submit(&submissionData,1);
    finished->waitForValue(1);

    unsigned char* colorPtr = outputBuffer->as<unsigned char>();
    auto groundTruth = utilities::loadTexelsFromFile("resources/textures/draw-triangle-test-result.png");
    GTEST_ASSERT_EQ(outputBuffer->countAsArray<unsigned char>(),groundTruth.size());

    for (auto i=0; i< outputBuffer->countAsArray<unsigned char>(); i++)
    {
        GTEST_ASSERT_EQ(colorPtr[i],groundTruth[i]);
    }
}

TEST_F(CommandBufferTest, DrawIndirectCount)
{
    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::GRAPHICS));
    std::unique_ptr<Semaphore> finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    std::unique_ptr<Buffer> globalsBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(GlobalSet0Group),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_BUFFER));
    std::unique_ptr<Buffer> objectBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(TexturedDepthSet1Group),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_BUFFER));
    std::unique_ptr<Texture> objectTexture = utilities::loadTextureFromFile("resources/textures/gradient.jpg");
    std::unique_ptr<Texture> target = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::RENDER_TARGET_ATTACHMENT,150,150,1,1,1));
    std::unique_ptr<Texture> depth = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::D24_UNORM_S8_UINT,Texture::Type::TEXTURE_2D,Texture::UsageFlags::DEPTH_STENCIL_ATTACHMENT,150,150,1,1,1));
    std::unique_ptr<Buffer> outputBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(target->byteSize(),Buffer::Accessibility::CPU_AND_GPU));
    std::unique_ptr<DescriptorPool> descriptorPool = std::unique_ptr<DescriptorPool>(DescriptorPool::newDescriptorPool());

    commandBuffer->begin();
    descriptorPool->reset();
    commandBuffer->bindDescriptorPool(descriptorPool.get());
    auto globalBundle = descriptorPool->makeBundle(TexturedDepthPipeline->descriptorGroup(0));
    auto objectBundle = descriptorPool->makeBundle(TexturedDepthPipeline->descriptorGroup(1));
    auto globals = globalsBuffer->as<GlobalSet0Group>();
    auto proj = glm::perspective(95.0f,(float)target->width()/(float)target->height(),.01f,100.0f);
    glm::mat4 view = glm::mat4(1.0f);
    view = glm::translate(view,glm::vec3(0.0f,2.0f,5.0f));
    view = glm::rotate(view,glm::radians(-20.0f),glm::vec3(1.0f,0.0f,0.0f));
    view = glm::inverse(view);//does nothing in this case, but is good practice if we ever do have a camera not at the default location
    glm::mat4 projectionView = proj*view;
    globals->projection = proj;
    globals->view = view;
    globals->projectionView = projectionView;
    auto object = objectBuffer->as<TexturedDepthSet1Group>();
    object->position = glm::rotate(glm::mat4(1.0f),glm::radians(45.0f),glm::vec3(0.0f,1.0f,0.0f));
    globalBundle.setUniformBuffer(0,0,globalsBuffer.get(),0,sizeof(GlobalSet0Group));
    commandBuffer->bindGraphicsShaderPipeline(TexturedDepthPipeline.get());
    commandBuffer->bindGraphicsDescriptorBundle(0,globalBundle);
    objectBundle.setUniformBuffer(0,0,objectBuffer.get(),0,sizeof(TexturedDepthSet1Group));
    objectBundle.setSampledTexture(1,0,objectTexture.get());
    objectBundle.setSampler(2,0,DefaultSampler.get());
    commandBuffer->bindGraphicsDescriptorBundle(0,globalBundle);
    commandBuffer->bindGraphicsDescriptorBundle(1,objectBundle);
    Attachment colorAttachment{.texture = target.get(),.autoClear = true,.clearValue = ClearValue{.color = {.floats = {0,0,0,1}}}};
    Attachment depthAttachment{.texture = depth.get(),.autoClear = true,.clearValue = ClearValue{.depthStencil = {.depth = 1, .stencil = 0}}};
    commandBuffer->beginRendering(&colorAttachment,1,&depthAttachment,slag::Rectangle{.extent = {target->width(),target->height()}});

    Buffer* vertexBuffers[]
    {
        TriangleVerts.get(),
        TriangleUVs.get()
    };
    uint64_t vertexOffsets[]{0,0};
    uint64_t bufferStrides[2] = {sizeof(glm::vec3),sizeof(glm::vec2)};
    commandBuffer->bindVertexBuffers(0,vertexBuffers,vertexOffsets,bufferStrides,2);

    std::unique_ptr<Buffer> drawCommands = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(IndirectDrawCommand),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::INDIRECT_BUFFER));
    auto drawCommandPtr = drawCommands->as<IndirectDrawCommand>();
    drawCommandPtr->vertexCount = 3;
    drawCommandPtr->instanceCount = 1;
    drawCommandPtr->firstVertex = 0;
    drawCommandPtr->firstInstance = 0;

    uint32_t drawCount = 1;
    std::unique_ptr<Buffer> drawCountParams = std::unique_ptr<Buffer>(Buffer::newBuffer(&drawCount,sizeof(uint32_t),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::INDIRECT_BUFFER));
    commandBuffer->setViewPort(0,0,target->width(),target->height(),1,0);
    commandBuffer->setScissors(slag::Rectangle{.offset = {0,0},.extent = {target->width(),target->height()}});
    commandBuffer->drawIndirectCount(drawCommands.get(),0,drawCountParams.get(),0,1,sizeof(IndirectDrawCommand));

    commandBuffer->endRendering();

    commandBuffer->insertBarrier(TextureBarrier{.texture = target.get(), .accessBefore = BarrierAccessFlags::COLOR_ATTACHMENT_WRITE,.accessAfter = BarrierAccessFlags::TRANSFER_READ,.syncBefore = PipelineStageFlags::ALL_GRAPHICS, .syncAfter = PipelineStageFlags::TRANSFER});
    TextureBufferMapping copyData
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
         .textureExtent = {target->width(0),target->height(0),1}
    };
    commandBuffer->copyTextureToBuffer(target.get(),outputBuffer.get(),&copyData,1);

    commandBuffer->end();

    CommandBuffer* submitBuffers[1] = {commandBuffer.get()};
    SemaphoreValue signal{.semaphore = finished.get(), .value = 1};
    QueueSubmissionBatch submissionData
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = submitBuffers,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1,
    };
    slagGraphicsCard()->graphicsQueue()->submit(&submissionData,1);
    finished->waitForValue(1);

    unsigned char* colorPtr = outputBuffer->as<unsigned char>();
    auto groundTruth = utilities::loadTexelsFromFile("resources/textures/draw-triangle-test-result.png");
    GTEST_ASSERT_EQ(outputBuffer->countAsArray<unsigned char>(),groundTruth.size());

    for (auto i=0; i< outputBuffer->countAsArray<unsigned char>(); i++)
    {
        GTEST_ASSERT_EQ(colorPtr[i],groundTruth[i]);
    }
}

TEST_F(CommandBufferTest, Dispatch)
{
    ShaderFile file{.pathIndicator = "resources/shaders/ParallelAdd", .stage = ShaderStageFlags::COMPUTE};
    auto compute = GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(file);
    std::unique_ptr<DescriptorPool> pool = std::unique_ptr<DescriptorPool>(DescriptorPool::newDescriptorPool());
    std::vector<float>inputs(128);
    for (int i=0; i<128; i++)
    {
        inputs[i] = i;
    }
    std::unique_ptr<Buffer> inputsBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(inputs.data(),128*sizeof(float),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::STORAGE_BUFFER));
    std::unique_ptr<Buffer> outputBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(64*sizeof(float),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::STORAGE_BUFFER));
    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::COMPUTE));
    std::unique_ptr<Semaphore> finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));

    commandBuffer->begin();
    commandBuffer->bindDescriptorPool(pool.get());
    auto parameters = pool->makeBundle(compute->descriptorGroup(0));
    parameters.setStorageBuffer(0,0,inputsBuffer.get(),0,64*sizeof(float));
    parameters.setStorageBuffer(1,0,inputsBuffer.get(),64*sizeof(float),64*sizeof(float));
    parameters.setStorageBuffer(2,0,outputBuffer.get(),0,64*sizeof(float));
    commandBuffer->bindComputeShaderPipeline(compute.get());
    commandBuffer->bindComputeDescriptorBundle(0,parameters);

    commandBuffer->dispatch(64,1,1);

    commandBuffer->end();

    CommandBuffer* submitBuffers[1] = {commandBuffer.get()};
    SemaphoreValue signal{.semaphore = finished.get(), .value = 1};
    QueueSubmissionBatch submissionData
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = submitBuffers,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1,
    };

    slagGraphicsCard()->computeQueue()->submit(&submissionData,1);

    finished->waitForValue(1);

    auto results = outputBuffer->as<float>();

    auto inputsData = inputsBuffer->as<float>();

    for (int i=0; i<64; i++)
    {
        GTEST_ASSERT_EQ(inputsData[i]+inputsData[i+64]+i,results[i]);
    }
}

TEST_F(CommandBufferTest, DispatchBase)
{
    ShaderFile file{.pathIndicator = "resources/shaders/ParallelAdd", .stage = ShaderStageFlags::COMPUTE};
    auto compute = GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(file);
    std::unique_ptr<DescriptorPool> pool = std::unique_ptr<DescriptorPool>(DescriptorPool::newDescriptorPool());
    std::vector<float>inputs(128);
    for (int i=0; i<128; i++)
    {
        inputs[i] = i;
    }
    std::vector<float>outputs(64,0.0f);
    std::unique_ptr<Buffer> inputsBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(inputs.data(),128*sizeof(float),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::STORAGE_BUFFER));
    std::unique_ptr<Buffer> outputBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(outputs.data(),64*sizeof(float),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::STORAGE_BUFFER));
    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::COMPUTE));
    std::unique_ptr<Semaphore> finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));

    commandBuffer->begin();
    commandBuffer->bindDescriptorPool(pool.get());
    auto parameters = pool->makeBundle(compute->descriptorGroup(0));
    parameters.setStorageBuffer(0,0,inputsBuffer.get(),0,64*sizeof(float));
    parameters.setStorageBuffer(1,0,inputsBuffer.get(),64*sizeof(float),64*sizeof(float));
    parameters.setStorageBuffer(2,0,outputBuffer.get(),0,64*sizeof(float));
    commandBuffer->bindComputeShaderPipeline(compute.get());
    commandBuffer->bindComputeDescriptorBundle(0,parameters);

    commandBuffer->dispatchBase(32,0,0,64,1,1);

    commandBuffer->end();

    CommandBuffer* submitBuffers[1] = {commandBuffer.get()};
    SemaphoreValue signal{.semaphore = finished.get(), .value = 1};
    QueueSubmissionBatch submissionData
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = submitBuffers,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1,
    };

    slagGraphicsCard()->computeQueue()->submit(&submissionData,1);

    finished->waitForValue(1);

    auto results = outputBuffer->as<float>();
    std::vector<float> floats(64);
    memcpy(floats.data(),results,floats.size()*sizeof(float));

    auto inputsData = inputsBuffer->as<float>();
    std::vector<float> floats2(128);
    memcpy(floats2.data(),inputsData,128*sizeof(float));


    for (int i=0; i<32; i++)
    {
        GTEST_ASSERT_EQ(0.0f,results[i]);
    }
    for (int i=32; i<64; i++)
    {
        GTEST_ASSERT_EQ(inputsData[i]+inputsData[i+64]+i,results[i]);
    }
}

TEST_F(CommandBufferTest, DispatchIndirect)
{
    ShaderFile file{.pathIndicator = "resources/shaders/ParallelAdd", .stage = ShaderStageFlags::COMPUTE};
    auto compute = GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(file);
    std::unique_ptr<DescriptorPool> pool = std::unique_ptr<DescriptorPool>(DescriptorPool::newDescriptorPool());
    std::vector<float>inputs(128);
    for (int i=0; i<128; i++)
    {
        inputs[i] = i;
    }
    std::unique_ptr<Buffer> inputsBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(inputs.data(),128*sizeof(float),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::STORAGE_BUFFER));
    std::unique_ptr<Buffer> outputBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(64*sizeof(float),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::STORAGE_BUFFER));
    IndirectDispatchCommand indirectDispatchCommand = {.groupCountX = 64,.groupCountY = 1,.groupCountZ = 1};
    std::unique_ptr<Buffer> indirectParameters = std::unique_ptr<Buffer>(Buffer::newBuffer(&indirectDispatchCommand,sizeof(IndirectDispatchCommand),Buffer::Accessibility::GPU,Buffer::UsageFlags::INDIRECT_BUFFER));
    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::COMPUTE));
    std::unique_ptr<Semaphore> finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));

    commandBuffer->begin();
    commandBuffer->bindDescriptorPool(pool.get());
    auto parameters = pool->makeBundle(compute->descriptorGroup(0));
    parameters.setStorageBuffer(0,0,inputsBuffer.get(),0,64*sizeof(float));
    parameters.setStorageBuffer(1,0,inputsBuffer.get(),64*sizeof(float),64*sizeof(float));
    parameters.setStorageBuffer(2,0,outputBuffer.get(),0,64*sizeof(float));
    commandBuffer->bindComputeShaderPipeline(compute.get());
    commandBuffer->bindComputeDescriptorBundle(0,parameters);

    commandBuffer->dispatchIndirect(indirectParameters.get(),0);

    commandBuffer->end();

    CommandBuffer* submitBuffers[1] = {commandBuffer.get()};
    SemaphoreValue signal{.semaphore = finished.get(), .value = 1};
    QueueSubmissionBatch submissionData
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = submitBuffers,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1,
    };

    slagGraphicsCard()->computeQueue()->submit(&submissionData,1);

    finished->waitForValue(1);

    auto results = outputBuffer->as<float>();

    auto inputsData = inputsBuffer->as<float>();

    for (int i=0; i<64; i++)
    {
        GTEST_ASSERT_EQ(inputsData[i]+inputsData[i+64]+i,results[i]);
    }
}
#endif