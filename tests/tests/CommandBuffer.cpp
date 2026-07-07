#include <gtest/gtest.h>
#include <slag/Slag.h>
#include "../utilities/GeneralUtilities.h"
#include "../utilities/SlagUtilities.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../utilities/PlatformUtilities.h"

using namespace slag;

TEST(CommandBuffer, Create)
{
    auto card = Slag::backend()->graphicsCard(0);
    auto queueType = sequentialEnumRange(QueueType::GRAPHICS,QueueType::TRANSFER);
    for (int i=0; i < queueType.size(); i++)
    {
        auto cb = std::unique_ptr<CommandBuffer>(card->newCommandBuffer(queueType[i]));
        GTEST_ASSERT_EQ(cb->graphicsCard(),card);
        GTEST_ASSERT_EQ(cb->type(),queueType[i]);
    }
}
//This test is really a "Trust me bro". There's no real way to check the barriers work, only that they don't work, so this really just checks for thrown errors
TEST(CommandBuffer, InsertBarriers)
{
    auto card = Slag::backend()->graphicsCard(0);
    auto commandBuffer = std::unique_ptr<CommandBuffer>(card->newCommandBuffer(QueueType::GRAPHICS));
    auto finished = std::unique_ptr<Semaphore>(card->newSemaphore());
    auto sourceBuffer = std::unique_ptr<Buffer>(card->newBuffer(256,BufferCPUAccess::WRITE_ONLY));
    auto destinationBuffer = std::unique_ptr<Buffer>(card->newBuffer(256,BufferCPUAccess::READ_WRITE));
    auto destinationTexture = std::unique_ptr<Texture>(card->newTexture2D(32,32,PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::SAMPLED));
    auto srcBufferPtr = sourceBuffer->as<uint8_t>();
    for (auto i=0; i< sourceBuffer->size(); i++)
    {
        srcBufferPtr[i] = i;
    }
    commandBuffer->begin();
    commandBuffer->copyBufferToBuffer(sourceBuffer.get(),0,destinationBuffer.get(),0,128);
    BufferBarrier bufferBarrier
    {
        .buffer = destinationBuffer.get(),
        .offset = 0,
        .length = 256,
        .syncBefore = SyncStages::COPY,
        .syncAfter = SyncStages::COPY,
        .flush = MemoryCaches::COPY_WRITE,
        .invalidate = MemoryCaches::NONE
    };
    commandBuffer->insertBarriers(&bufferBarrier,1);
    TextureBufferMapping mapping
    {
        .bufferOffset = 0,
        .subresource =
        {
            .aspect = PixelAspect::COLOR,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .offset = {0,0,0},
        .extent = {32,1,1}
    };
    commandBuffer->copyBufferToTexture(sourceBuffer.get(),destinationTexture.get(),&mapping,1);
    TextureBarrier textureBarrier
    {
        .texture = destinationTexture.get(),
        .syncBefore = SyncStages::COPY,
        .syncAfter = SyncStages::COPY,
        .flush = MemoryCaches::COPY_WRITE,
        .invalidate = MemoryCaches::NONE,
    };
    commandBuffer->insertBarriers(&textureBarrier,1);
    commandBuffer->copyBufferToBuffer(sourceBuffer.get(),0,destinationBuffer.get(),128,128);
    mapping.offset.y = 1;
    commandBuffer->copyBufferToTexture(sourceBuffer.get(),destinationTexture.get(),&mapping,1);
    GlobalBarrier globalBarrier
    {
        .syncBefore = SyncStages::COPY,
        .syncAfter = SyncStages::COPY,
        .flush = MemoryCaches::COPY_WRITE,
        .invalidate = MemoryCaches::COPY_READ,
    };
    commandBuffer->insertBarriers(&globalBarrier,1);
    mapping.offset.y = 2;
    commandBuffer->copyBufferToTexture(sourceBuffer.get(),destinationTexture.get(),&mapping,1);
    commandBuffer->insertBarriers(nullptr,0,nullptr,0,&textureBarrier,1);
    commandBuffer->end();

    SemaphoreValue signal{.semaphore = finished.get(), .value = 1};
    auto cb = commandBuffer.get();
    SubmissionBatch submissionBatch{};
    submissionBatch.commandBuffers = &cb;
    submissionBatch.commandBufferCount = 1;
    submissionBatch.signalSemaphores = &signal;
    submissionBatch.signalSemaphoreCount = 1;
    card->graphicsQueue()->submit(&submissionBatch,1);
    finished->waitForValue(1);

}

TEST(CommandBuffer, CopyBufferToBuffer)
{
    auto card = Slag::backend()->graphicsCard(0);
    auto commandBuffer = std::unique_ptr<CommandBuffer>(card->newCommandBuffer(QueueType::TRANSFER));
    auto srcBuffer = std::unique_ptr<Buffer>(card->newBuffer(128,BufferCPUAccess::WRITE_ONLY,BufferMemoryType::GENERAL));
    auto dstBuffer = std::unique_ptr<Buffer>(card->newBuffer(128,BufferCPUAccess::READ_WRITE,BufferMemoryType::GENERAL));
    auto finished = std::unique_ptr<Semaphore>(card->newSemaphore());

    auto srcPtr = srcBuffer->as<uint8_t>();
    auto dstPtr = dstBuffer->as<uint8_t>();

    for (uint8_t i = 0; i < 128; i++)
    {
        srcPtr[i] = i;
    }

    commandBuffer->begin();
    commandBuffer->copyBufferToBuffer(srcBuffer.get(),0,dstBuffer.get(),64,64);
    commandBuffer->copyBufferToBuffer(srcBuffer.get(),64,dstBuffer.get(),0,64);
    commandBuffer->end();

    auto cmdBufferPtr = commandBuffer.get();
    SemaphoreValue signal
    {
        .semaphore = finished.get(),
        .value = 1
    };
    SubmissionBatch batch
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = &cmdBufferPtr,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1,
    };

    card->transferQueue()->submit(&batch,1);
    finished->waitForValue(1);
    for (uint8_t i=0; i < 128; i++)
    {
        if (i<64)
        {
            GTEST_ASSERT_EQ(dstPtr[i+64],i);
        }
        else
        {
            GTEST_ASSERT_EQ(dstPtr[i-64],i);
        }
    }
}

TEST(CommandBuffer, CopyBufferToTextureToBuffer)
{
    auto card = Slag::backend()->graphicsCard(0);
    auto commandBuffer = std::unique_ptr<CommandBuffer>(card->newCommandBuffer(QueueType::TRANSFER));
    auto texture = std::unique_ptr<Texture>(card->newTexture2D(64,64,PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::SAMPLED,3));
    auto finished = std::unique_ptr<Semaphore>(card->newSemaphore());
    auto srcBuffer = std::unique_ptr<Buffer>(card->newBuffer((32*32*4)+32,BufferCPUAccess::WRITE_ONLY));
    auto dstBuffer = std::unique_ptr<Buffer>(card->newBuffer(srcBuffer->size(),BufferCPUAccess::READ_WRITE));
    auto srcBufferPtr = srcBuffer->as<uint8_t>();
    for (auto i=16; i<srcBuffer->size()-16; i++)
    {
        srcBufferPtr[i] = 172;
    }
    auto dstPtr = dstBuffer->as<uint8_t>();
    for (auto i=0; i<dstBuffer->size(); i++)
    {
        dstPtr[i] = 0;
    }

    commandBuffer->begin();
    TextureBufferMapping mapping
    {
        .bufferOffset = 16,
        .subresource =
        {
            .aspect = PixelAspect::COLOR,
            .mipLevel = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .offset = {0,0,0},
        .extent = {32,32,1}
    };
    commandBuffer->copyBufferToTexture(srcBuffer.get(),texture.get(),&mapping,1);
    TextureBarrier textureBarrier
    {
        .texture = texture.get(),
        .syncBefore = SyncStages::COPY,
        .syncAfter = SyncStages::COPY,
        .flush = MemoryCaches::COPY_WRITE,
        .invalidate = MemoryCaches::COPY_READ,
    };
    commandBuffer->insertBarriers(&textureBarrier,1);
    commandBuffer->copyTextureToBuffer(texture.get(),dstBuffer.get(),&mapping,1);
    commandBuffer->end();

    auto cb = commandBuffer.get();
    SemaphoreValue signal{.semaphore = finished.get(),.value = 1};
    SubmissionBatch batch{};
    batch.commandBuffers = &cb;
    batch.commandBufferCount = 1;
    batch.signalSemaphores = &signal;
    batch.signalSemaphoreCount = 1;
    card->transferQueue()->submit(&batch,1);
    finished->waitForValue(1);

    for (int i=0; i< dstBuffer->size(); i++)
    {
        if (i<16 || i>= dstBuffer->size()-16)
        {
            GTEST_ASSERT_EQ(dstPtr[i],0);
        }
        else
        {
            GTEST_ASSERT_EQ(dstPtr[i],172);
        }
    }
}

TEST(CommandBuffer, Draw)
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
    auto colorTexture = std::unique_ptr<Texture>(graphicsCard->newTexture2D(256,256,PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::COLOR_TARGET));
    auto depthTexture = std::unique_ptr<Texture>(graphicsCard->newTexture2D(256,256,PixelFormat::D32_FLOAT,TextureUsageFlags::DEPTH_STENCIL_TARGET));

    auto positionBuffer = std::unique_ptr<Buffer>(graphicsCard->newBuffer(sizeof(positions),BufferCPUAccess::WRITE_ONLY));
    auto uvBuffer = std::unique_ptr<Buffer>(graphicsCard->newBuffer(sizeof(uvs),BufferCPUAccess::WRITE_ONLY));

    memcpy(positionBuffer->as<glm::vec3>(),positions,sizeof(positions));
    memcpy(uvBuffer->as<glm::vec2>(),uvs,sizeof(uvs));

    auto globalsBuffer = std::unique_ptr<Buffer>(graphicsCard->newBuffer(256,BufferCPUAccess::WRITE_ONLY,BufferMemoryType::UNIFORM));
    auto instanceBuffer = std::unique_ptr<Buffer>(graphicsCard->newBuffer(256,BufferCPUAccess::WRITE_ONLY,BufferMemoryType::UNIFORM));
    auto instanceTexture = utilities::loadTexture("resources/tests/textures/gradient.jpg", graphicsCard);
    auto sampler = std::unique_ptr<Sampler>(graphicsCard->newSampler());

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
    auto vertex = utilities::createShaderModule(graphicsCard,"resources/tests/shaders/compiled/TexturedDepthBindless.vertex");
    auto fragment = utilities::createShaderModule(graphicsCard,"resources/tests/shaders/compiled/TexturedDepthBindless.fragment");
    std::vector<VertexBinding> vertexBindings =
    {
        VertexBinding(0,sizeof(float)*3,InputRate::PER_VRETEX,std::vector<VertexAttribute>{VertexAttribute("POSITION",PixelFormat::R32G32B32_FLOAT,0)}),
        VertexBinding(1,sizeof(float)*2,InputRate::PER_VRETEX,std::vector<VertexAttribute>{VertexAttribute("UV_COORDINATES",PixelFormat::R32G32_FLOAT,0)}),
    };
    VertexDescription vertexDescription(vertexBindings);
    FramebufferDescription framebufferDesc{};
    framebufferDesc.colorFormats[0] = colorTexture->format();
    framebufferDesc.depthFormat = depthTexture->format();
    auto shaderPipeline = std::unique_ptr<ShaderPipeline>(graphicsCard->newShaderPipeline(vertexDescription,vertex.details,fragment.details,pipelineState,framebufferDesc));

    auto resourceHeap = std::unique_ptr<ResourceDescriptorHeap>(graphicsCard->newResourceDescriptorHeap(512));
    auto samplerHeap = std::unique_ptr<SamplerDescriptorHeap>(graphicsCard->newSamplerDescriptorHeap(512));

    auto heapDetails = graphicsCard->descriptorHeapDetails();
    resourceHeap->setUniformBuffer(0,globalsBuffer.get(),0,globalsBuffer->size());
    resourceHeap->setUniformBuffer(heapDetails.bufferDescriptorSize,instanceBuffer.get(),0,instanceBuffer->size());
    resourceHeap->setUniformTexture(heapDetails.textureDescriptorSize*2,instanceTexture.get(),0,1,0,1);

    samplerHeap->setSampler(0,sampler.get());

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

    commandBuffer->bindGraphicsPipeline(shaderPipeline.get());
    Buffer* vertexBuffers[] = {positionBuffer.get(),uvBuffer.get()};
    uint64_t offsets[] = {0,0};
    uint64_t strides[] = {sizeof(glm::vec3),sizeof(glm::vec2)};
    commandBuffer->bindVertexBuffers(0,vertexBuffers,offsets,strides,2);

    Attachment colorAttachment(colorTexture.get(),true,{.color = ClearColor{.floats = {0.0f,0.0f,1.0f,1.0f}}});
    Attachment depthAttachment(depthTexture.get(),true,{.depthStencil = ClearDepthStencilValue{.depth = 1.0f, .stencil = 0}});

    TextureBarrier barriers[] =
    {
        TextureBarrier
        {
            .texture = colorTexture.get(),
            .baseLayer = 0,
            .layerCount = 1,
            .baseMipLevel = 0,
            .mipCount = 1,
            .syncBefore = SyncStages::ALL,
            .syncAfter = SyncStages::ALL_GRAPHICS,
            .flush = MemoryCaches::NONE,
            .invalidate = MemoryCaches::NONE,
            .layoutBefore = TextureLayout::GENERAL,
            .layoutAfter = TextureLayout::COLOR_TARGET,
        },
        TextureBarrier
        {
            .texture = depthTexture.get(),
            .baseLayer = 0,
            .layerCount = 1,
            .baseMipLevel = 0,
            .mipCount = 1,
            .syncBefore = SyncStages::ALL,
            .syncAfter = SyncStages::ALL_GRAPHICS,
            .flush = MemoryCaches::NONE,
            .invalidate = MemoryCaches::NONE,
            .layoutBefore = TextureLayout::GENERAL,
            .layoutAfter = TextureLayout::DEPTH_STENCIL_TARGET,
        }
    };
    commandBuffer->insertBarriers(barriers,2);

    commandBuffer->beginRendering(&colorAttachment,1,&depthAttachment,slag::Rectangle{0,0,colorTexture->width(),colorTexture->height()});
    commandBuffer->setViewPort(0,0,colorTexture->width(),colorTexture->height(),.0f,1.0f);
    commandBuffer->setScissors(slag::Rectangle{0,0,colorTexture->width(),colorTexture->height()});
    commandBuffer->draw(6,1,0,0);
    commandBuffer->endRendering();

    barriers[0].layoutBefore = TextureLayout::COLOR_TARGET;
    barriers[0].layoutAfter = TextureLayout::GENERAL;
    barriers[0].syncAfter = SyncStages::ALL;
    barriers[0].flush = MemoryCaches::COLOR_TARGET;
    barriers[0].invalidate = MemoryCaches::COPY_READ;

    commandBuffer->insertBarriers(barriers,1);

    commandBuffer->end();
    auto cmdBuffer = commandBuffer.get();
    SemaphoreValue signal{.semaphore = finished.get(),.value = 1};
    SubmissionBatch batch
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = &cmdBuffer,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1,
    };
    graphicsCard->graphicsQueue()->submit(&batch,1);
    finished->waitForValue(1);
    auto result = utilities::compareTexture(colorTexture.get(),0,0,"resources/tests/textures/results/draw-test.png");
    ASSERT_GE(result.overallSimilarity,.9999);
}

TEST(CommandBuffer, DrawIndexed)
{
    glm::vec3 positions[]
    {
        {-1,1,0},
        {1,1,0},
        {-1,-1,0},
        {1,-1,0}
    };
    glm::vec2 uvs[]
    {
        {0,0},
        {1,0},
        {0,1},
        {1,1}
    };
    uint16_t indices[]
    {
        0,1,2,
        1,3,2
    };
    auto graphicsCard = Slag::backend()->graphicsCard(0);
    auto commandBuffer = std::unique_ptr<CommandBuffer>(graphicsCard->newCommandBuffer(QueueType::GRAPHICS));
    auto finished = std::unique_ptr<Semaphore>(graphicsCard->newSemaphore());
    auto colorTexture = std::unique_ptr<Texture>(graphicsCard->newTexture2D(256,256,PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::COLOR_TARGET));
    auto depthTexture = std::unique_ptr<Texture>(graphicsCard->newTexture2D(256,256,PixelFormat::D32_FLOAT,TextureUsageFlags::DEPTH_STENCIL_TARGET));

    auto positionBuffer = std::unique_ptr<Buffer>(graphicsCard->newBuffer(sizeof(positions),BufferCPUAccess::WRITE_ONLY));
    auto uvBuffer = std::unique_ptr<Buffer>(graphicsCard->newBuffer(sizeof(uvs),BufferCPUAccess::WRITE_ONLY));
    auto indexBuffer = std::unique_ptr<Buffer>(graphicsCard->newBuffer(sizeof(indices),BufferCPUAccess::WRITE_ONLY));

    memcpy(positionBuffer->as<glm::vec3>(),positions,sizeof(positions));
    memcpy(uvBuffer->as<glm::vec2>(),uvs,sizeof(uvs));
    memcpy(indexBuffer->as<uint16_t>(),indices,sizeof(indices));

    auto globalsBuffer = std::unique_ptr<Buffer>(graphicsCard->newBuffer(256,BufferCPUAccess::WRITE_ONLY,BufferMemoryType::UNIFORM));
    auto instanceBuffer = std::unique_ptr<Buffer>(graphicsCard->newBuffer(256,BufferCPUAccess::WRITE_ONLY,BufferMemoryType::UNIFORM));
    auto instanceTexture = utilities::loadTexture("resources/tests/textures/gradient.jpg", graphicsCard);
    auto sampler = std::unique_ptr<Sampler>(graphicsCard->newSampler());

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
    auto vertex = utilities::createShaderModule(graphicsCard,"resources/tests/shaders/compiled/TexturedDepthBindless.vertex");
    auto fragment = utilities::createShaderModule(graphicsCard,"resources/tests/shaders/compiled/TexturedDepthBindless.fragment");
    std::vector<VertexBinding> vertexBindings =
    {
        VertexBinding(0,sizeof(float)*3,InputRate::PER_VRETEX,std::vector<VertexAttribute>{VertexAttribute("POSITION",PixelFormat::R32G32B32_FLOAT,0)}),
        VertexBinding(1,sizeof(float)*2,InputRate::PER_VRETEX,std::vector<VertexAttribute>{VertexAttribute("UV_COORDINATES",PixelFormat::R32G32_FLOAT,0)}),
    };
    VertexDescription vertexDescription(vertexBindings);
    FramebufferDescription framebufferDesc{};
    framebufferDesc.colorFormats[0] = colorTexture->format();
    framebufferDesc.depthFormat = depthTexture->format();
    auto shaderPipeline = std::unique_ptr<ShaderPipeline>(graphicsCard->newShaderPipeline(vertexDescription,vertex.details,fragment.details,pipelineState,framebufferDesc));

    auto resourceHeap = std::unique_ptr<ResourceDescriptorHeap>(graphicsCard->newResourceDescriptorHeap(512));
    auto samplerHeap = std::unique_ptr<SamplerDescriptorHeap>(graphicsCard->newSamplerDescriptorHeap(512));

    auto heapDetails = graphicsCard->descriptorHeapDetails();
    resourceHeap->setUniformBuffer(0,globalsBuffer.get(),0,globalsBuffer->size());
    resourceHeap->setUniformBuffer(heapDetails.bufferDescriptorSize,instanceBuffer.get(),0,instanceBuffer->size());
    resourceHeap->setUniformTexture(heapDetails.textureDescriptorSize*2,instanceTexture.get(),0,1,0,1);

    samplerHeap->setSampler(0,sampler.get());

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

    commandBuffer->bindGraphicsPipeline(shaderPipeline.get());
    Buffer* vertexBuffers[] = {positionBuffer.get(),uvBuffer.get()};
    uint64_t offsets[] = {0,0};
    uint64_t strides[] = {sizeof(glm::vec3),sizeof(glm::vec2)};
    commandBuffer->bindVertexBuffers(0,vertexBuffers,offsets,strides,2);
    commandBuffer->bindIndexBuffer(indexBuffer.get(),IndexBufferType::UINT_16,0);

    Attachment colorAttachment(colorTexture.get(),true,{.color = ClearColor{.floats = {0.0f,0.0f,1.0f,1.0f}}});
    Attachment depthAttachment(depthTexture.get(),true,{.depthStencil = ClearDepthStencilValue{.depth = 1.0f, .stencil = 0}});

    TextureBarrier barriers[] =
    {
        TextureBarrier
        {
            .texture = colorTexture.get(),
            .baseLayer = 0,
            .layerCount = 1,
            .baseMipLevel = 0,
            .mipCount = 1,
            .syncBefore = SyncStages::ALL,
            .syncAfter = SyncStages::ALL_GRAPHICS,
            .flush = MemoryCaches::NONE,
            .invalidate = MemoryCaches::NONE,
            .layoutBefore = TextureLayout::GENERAL,
            .layoutAfter = TextureLayout::COLOR_TARGET,
        },
        TextureBarrier
        {
            .texture = depthTexture.get(),
            .baseLayer = 0,
            .layerCount = 1,
            .baseMipLevel = 0,
            .mipCount = 1,
            .syncBefore = SyncStages::ALL,
            .syncAfter = SyncStages::ALL_GRAPHICS,
            .flush = MemoryCaches::NONE,
            .invalidate = MemoryCaches::NONE,
            .layoutBefore = TextureLayout::GENERAL,
            .layoutAfter = TextureLayout::DEPTH_STENCIL_TARGET,
        }
    };
    commandBuffer->insertBarriers(barriers,2);

    commandBuffer->beginRendering(&colorAttachment,1,&depthAttachment,slag::Rectangle{0,0,colorTexture->width(),colorTexture->height()});
    commandBuffer->setViewPort(0,0,colorTexture->width(),colorTexture->height(),.0f,1.0f);
    commandBuffer->setScissors(slag::Rectangle{0,0,colorTexture->width(),colorTexture->height()});
    commandBuffer->drawIndexed(6,1,0,0,0);
    commandBuffer->endRendering();

    barriers[0].layoutBefore = TextureLayout::COLOR_TARGET;
    barriers[0].layoutAfter = TextureLayout::GENERAL;
    barriers[0].syncAfter = SyncStages::ALL;
    barriers[0].flush = MemoryCaches::COLOR_TARGET;
    barriers[0].invalidate = MemoryCaches::COPY_READ;

    commandBuffer->insertBarriers(barriers,1);

    commandBuffer->end();
    auto cmdBuffer = commandBuffer.get();
    SemaphoreValue signal{.semaphore = finished.get(),.value = 1};
    SubmissionBatch batch
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = &cmdBuffer,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1,
    };
    graphicsCard->graphicsQueue()->submit(&batch,1);
    finished->waitForValue(1);
    auto result = utilities::compareTexture(colorTexture.get(),0,0,"resources/tests/textures/results/draw-test.png");
    ASSERT_GE(result.overallSimilarity,.9999);
}


#ifdef SLAG_DEBUG
TEST(CommandBuffer, TransferErrorComputeCommands)
{
    GTEST_FAIL();
}
TEST(CommandBuffer, TransferErrorGraphicsCommands)
{
    GTEST_FAIL();
}
TEST(CommandBuffer, ComputeErrorGraphicsCommands)
{
    GTEST_FAIL();
}
#endif
