#include <gtest/gtest.h>
#include <slag/Slag.h>
#include "../utilities/GeneralUtilities.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../utilities/GeneralUtilities.h"
#include "../utilities/SlagUtilities.h"
#include "../utilities/PlatformUtilities.h"
using namespace slag;

TEST(FrameBufferView, Texture1D)
{
    GTEST_FAIL();
}
TEST(FrameBufferView, Texture1DArray)
{
    GTEST_FAIL();
}

TEST(FrameBufferView, Texture2D)
{
    glm::vec3 positions[]
    {
        {-1,1,0},
        {1,1,0},
        {-1,-1,0},
        {1,1,0},
        {1,-1,0},
        {-1,-1,0}
    };
    glm::vec2 uvs[]
    {
        {0,0},
        {1,0},
        {0,1},
        {1,0},
        {1,1},
        {0,1}
    };

    auto graphicsCard = Slag::backend()->graphicsCard(0);
    auto commandBuffer = std::unique_ptr<CommandBuffer>(graphicsCard->newCommandBuffer(QueueType::GRAPHICS));
    auto finished = std::unique_ptr<Semaphore>(graphicsCard->newSemaphore());

    auto positionBuffer = std::unique_ptr<Buffer>(graphicsCard->newBuffer(sizeof(positions),BufferCPUAccess::WRITE_ONLY));
    auto uvBuffer = std::unique_ptr<Buffer>(graphicsCard->newBuffer(sizeof(uvs),BufferCPUAccess::WRITE_ONLY));

    memcpy(positionBuffer->as<glm::vec3>(),positions,sizeof(positions));
    memcpy(uvBuffer->as<glm::vec2>(),uvs,sizeof(uvs));

    auto globalsBuffer = std::unique_ptr<Buffer>(graphicsCard->newBuffer(256,BufferCPUAccess::WRITE_ONLY,BufferMemoryType::UNIFORM));
    auto instanceBuffer = std::unique_ptr<Buffer>(graphicsCard->newBuffer(256,BufferCPUAccess::WRITE_ONLY,BufferMemoryType::UNIFORM));
    auto instanceTexture = utilities::loadTexture("resources/tests/textures/gradient.jpg", graphicsCard);
    auto sampler = std::unique_ptr<Sampler>(graphicsCard->newSampler());

    auto colorTexture = std::unique_ptr<Texture>(graphicsCard->newTexture2D(PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::COLOR_TARGET,512,512,3));
    auto colorView = std::unique_ptr<FrameBufferView>(graphicsCard->newFrameBufferView(colorTexture.get(),1));
    auto textureData = std::unique_ptr<Buffer>(graphicsCard->newBuffer(colorTexture->bufferSize(PixelAspect::COLOR),BufferCPUAccess::WRITE_ONLY));
    auto textureDataPtr = textureData->as<uint8_t>();
    for (auto i=0; i< textureData->size(); i+=4)
    {
        textureDataPtr[i] = 0;
        textureDataPtr[i+1] = 0;
        textureDataPtr[i+2] = 0;
        textureDataPtr[i+3] = 255;
    }

    auto globalsPtr = globalsBuffer->as<glm::mat4>();
    auto proj = glm::perspective(95.0f,(float)colorTexture->width()/(float)colorTexture->height(),.01f,100.0f);
    glm::mat4 view = glm::mat4(1.0f);
    view = glm::translate(view,glm::vec3(0.0f,2.0f,5.0f));
    view = glm::rotate(view,glm::radians(-20.0f),glm::vec3(1.0f,0.0f,0.0f));
    view = glm::inverse(view);
    glm::mat4 projectionView = proj*view;
    globalsPtr[0] = proj;
    globalsPtr[1] = view;
    globalsPtr[2] = projectionView;

    auto instancePtr = instanceBuffer->as<glm::mat4>();
    glm::mat4 instanceTransform = glm::rotate(glm::mat4(1.0f),glm::radians(15.0f),glm::vec3(0.0f,1.0f,0.0f));
    instancePtr[0] = instanceTransform;

    PipelineState pipelineState{};
    pipelineState.depthStencilState.depthTestEnable = false;
    auto vertex = utilities::createShaderModule(graphicsCard,"resources/tests/shaders/compiled/TexturedDepthBindless.vertex");
    auto fragment = utilities::createShaderModule(graphicsCard,"resources/tests/shaders/compiled/TexturedDepthBindless.fragment");
    std::vector<VertexBinding> vertexBindings =
    {
        VertexBinding(0,sizeof(float)*3,InputRate::PER_VERTEX,std::vector<VertexAttribute>{VertexAttribute("POSITION",PixelFormat::R32G32B32_FLOAT,0)}),
        VertexBinding(1,sizeof(float)*2,InputRate::PER_VERTEX,std::vector<VertexAttribute>{VertexAttribute("UV_COORDINATES",PixelFormat::R32G32_FLOAT,0)}),
    };
    VertexDescription vertexDescription(vertexBindings);
    FramebufferDescription framebufferDesc{};
    framebufferDesc.colorFormats[0] = colorTexture->format();
    framebufferDesc.depthFormat = PixelFormat::UNDEFINED;
    auto shaderPipeline = std::unique_ptr<ShaderPipeline>(graphicsCard->newShaderPipeline(vertexDescription,vertex.details,fragment.details,pipelineState,framebufferDesc));

    auto resourceHeap = std::unique_ptr<ResourceDescriptorHeap>(graphicsCard->newResourceDescriptorHeap(512));
    auto samplerHeap = std::unique_ptr<SamplerDescriptorHeap>(graphicsCard->newSamplerDescriptorHeap(512));

    auto resourceHeapPtr = (unsigned char*)resourceHeap->data();
    auto samplerHeapPtr = (unsigned char*)samplerHeap->data();

    auto heapDetails = graphicsCard->descriptorHeapDetails();

    graphicsCard->writeUniformBufferDescriptor(globalsBuffer.get(),0,globalsBuffer->size(),resourceHeapPtr);
    graphicsCard->writeUniformBufferDescriptor(instanceBuffer.get(),0,instanceBuffer->size(),resourceHeapPtr+heapDetails.textureDescriptorSize);
    graphicsCard->writeUniformTextureDescriptor(instanceTexture.get(),0,1,0,1,resourceHeapPtr+heapDetails.textureDescriptorSize*2);
    graphicsCard->writeSamplerDescriptor(sampler.get(),samplerHeapPtr);



    TextureBufferMapping mappings[]
    {
        {
            .bufferOffset = 0,
            .subresource = TextureSubresource
            {
                .aspect = PixelAspect::COLOR,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .offset = {0,0,0},
            .extent = {colorTexture->width(),colorTexture->height(),1}
        },
        {
            .bufferOffset = 512*512*4,
            .subresource = TextureSubresource
            {
                .aspect = PixelAspect::COLOR,
                .mipLevel = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .offset = {0,0,0},
            .extent = {colorTexture->mipWidth(1),colorTexture->mipHeight(1),1}
        },
        {
            .bufferOffset = 512*512*4 + 256*256*4,
            .subresource = TextureSubresource
            {
                .aspect = PixelAspect::COLOR,
                .mipLevel = 2,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .offset = {0,0,0},
            .extent = {colorTexture->mipWidth(2),colorTexture->mipHeight(2),1}
        }
    };

    commandBuffer->begin();

    commandBuffer->bindDescriptorHeaps(resourceHeap.get(),samplerHeap.get());

    uint32_t globalsIndex = 0;
    uint32_t instanceIndex = 1;
    uint32_t textureIndex = 2;
    uint32_t samplerIndex = 0;
    commandBuffer->setGraphicsShaderParameters(0,&globalsIndex,sizeof(uint32_t));
    commandBuffer->setGraphicsShaderParameters(8,&instanceIndex,sizeof(uint32_t));
    commandBuffer->setGraphicsShaderParameters(16,&textureIndex,sizeof(uint32_t));
    commandBuffer->setGraphicsShaderParameters(24,&samplerIndex,sizeof(uint32_t));

    commandBuffer->bindShaderPipeline(shaderPipeline.get());
    Buffer* vertexBuffers[] = {positionBuffer.get(),uvBuffer.get()};
    uint64_t offsets[] = {0,0};
    uint64_t strides[] = {sizeof(glm::vec3),sizeof(glm::vec2)};
    commandBuffer->bindVertexBuffers(0,vertexBuffers,offsets,strides,2);

    commandBuffer->copyBufferToTexture(textureData.get(),colorTexture.get(),mappings,3);
    TextureBarrier textureBarrier
    {
        .texture = colorTexture.get(),
        .baseLayer = 0,
        .layerCount = colorTexture->layers(),
        .baseMipLevel = 0,
        .mipCount = colorTexture->mipLevels(),
        .syncBefore = SyncStages::COPY,
        .syncAfter = SyncStages::ALL_GRAPHICS,
        .flush = MemoryCaches::COPY_WRITE,
        .invalidate = MemoryCaches::COLOR_TARGET,
        .layoutBefore = TextureLayout::GENERAL,
        .layoutAfter = TextureLayout::COLOR_TARGET
    };
    commandBuffer->insertBarriers(&textureBarrier,1);

    Attachment colorAttachment{.bufferView = colorView.get(),.autoClear = true, .clearValue = {0.0f,0,1.0f,1.0f}};
    commandBuffer->beginRendering(&colorAttachment,1,nullptr,slag::Rectangle{{0,0},{colorTexture->mipWidth(1),colorTexture->mipHeight(1)}});
    commandBuffer->setViewPort(0,0,colorTexture->mipWidth(1),colorTexture->mipHeight(1),.0f,1.0f);
    commandBuffer->setScissors(slag::Rectangle{0,0,colorTexture->mipWidth(1),colorTexture->mipHeight(1)});
    commandBuffer->draw(6,1,0,0);
    commandBuffer->endRendering();
    textureBarrier.syncBefore = SyncStages::ALL_GRAPHICS;
    textureBarrier.syncAfter = SyncStages::COPY;
    textureBarrier.flush = MemoryCaches::COLOR_TARGET;
    textureBarrier.invalidate = MemoryCaches::COPY_READ;
    textureBarrier.layoutBefore = TextureLayout::COLOR_TARGET;
    textureBarrier.layoutAfter = TextureLayout::GENERAL;
    commandBuffer->insertBarriers(&textureBarrier,1);


    commandBuffer->end();

    SemaphoreValue signal{.semaphore = finished.get(), .value = 1};
    CommandBuffer* cb = commandBuffer.get();
    graphicsCard->graphicsQueue()->submit({.waitSemaphores = nullptr, .waitSemaphoreCount = 0, .commandBuffers = &cb, .commandBufferCount = 1, .signalSemaphores = &signal, .signalSemaphoreCount = 1});

    finished->waitForValue(1);

    auto mip0 = utilities::compareTexture(colorTexture.get(),0,0,"resources/tests/textures/results/FrameBufferViewMip0.png");
    GTEST_ASSERT_GE(mip0.overallSimilarity,.9999f);
    auto mip1 = utilities::compareTexture(colorTexture.get(),0,1,"resources/tests/textures/results/FrameBufferViewMip1.png");
    GTEST_ASSERT_GE(mip1.overallSimilarity,.9999f);
    auto mip2 = utilities::compareTexture(colorTexture.get(),0,2,"resources/tests/textures/results/FrameBufferViewMip2.png");
    GTEST_ASSERT_GE(mip2.overallSimilarity,.9999f);

}

TEST(FrameBufferView, Texture2DArray)
{
    GTEST_FAIL();
}

TEST(FrameBufferView, Texture2DMultiSampled)
{
    GTEST_FAIL();
}

TEST(FrameBufferView, Texture2DArrayMultiSampled)
{
    GTEST_FAIL();
}

TEST(FrameBufferView, Texture3D)
{
    GTEST_FAIL();
}

TEST(FrameBufferView, CubeMap)
{
    GTEST_FAIL();
}

TEST(FrameBufferView, Depth1D)
{
    GTEST_FAIL();
}

TEST(FrameBufferView, Depth1DArray)
{
    GTEST_FAIL();
}

TEST(FrameBufferView, Depth2D)
{
    GTEST_FAIL();
}
TEST(FrameBufferView, Depth2DArray)
{
    GTEST_FAIL();
}

TEST(FrameBufferView, Depth2DMultiSampled)
{
    GTEST_FAIL();
}
TEST(FrameBufferView, Depth2DArrayMultiSampled)
{
    GTEST_FAIL();
}

#ifdef SLAG_DEBUG
TEST(FrameBufferView, FailIfTargetFlagsNotSet)
{
    auto graphicsCard = Slag::backend()->graphicsCard(0);
    auto colorTexture = std::unique_ptr<Texture>(graphicsCard->newTexture2D(PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::SAMPLED,500,500));
    auto depthTexture = std::unique_ptr<Texture>(graphicsCard->newTexture2D(PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::NONE,500,500));

    EXPECT_DEATH(auto colorView = std::unique_ptr<FrameBufferView>(graphicsCard->newFrameBufferView(colorTexture.get())),"Texture must have either COLOR_TARGET or DEPTH_STENCIL_TARGET usage flags");
    EXPECT_DEATH(auto depthView = std::unique_ptr<FrameBufferView>(graphicsCard->newFrameBufferView(depthTexture.get())),"Texture must have either COLOR_TARGET or DEPTH_STENCIL_TARGET usage flags");

}
TEST(FrameBufferView, FailIfParametersBeyondBounds)
{
    auto graphicsCard = Slag::backend()->graphicsCard(0);
    auto colorTexture = std::unique_ptr<Texture>(graphicsCard->newTexture2D(PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::COLOR_TARGET,500,500,1,2));
    EXPECT_DEATH(auto colorView = std::unique_ptr<FrameBufferView>(graphicsCard->newFrameBufferView(colorTexture.get(),2,0,1)),"Cannot assign a texture view into resources beyond the texture bounds");
    EXPECT_DEATH(auto colorView = std::unique_ptr<FrameBufferView>(graphicsCard->newFrameBufferView(colorTexture.get(),1,2,1)),"Cannot assign a texture view into resources beyond the texture bounds");
    EXPECT_DEATH(auto colorView = std::unique_ptr<FrameBufferView>(graphicsCard->newFrameBufferView(colorTexture.get(),1,1,2)),"Cannot assign a texture view into resources beyond the texture bounds");
}
#endif
