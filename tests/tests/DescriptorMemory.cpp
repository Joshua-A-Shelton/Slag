#include <gtest/gtest.h>
#include <slag/Slag.h>
#include "../Utilities.h"
#include "../GraphicsAPIEnvironment.h"
#include "glm/vec4.hpp"

using namespace slag;
float DESCRIPTOR_MEMORY_TRIANGLE_DATA[]={-1.0f,-1.0f,0,1.0f,1.0f,-1.0f};
float DESCRIPTOR_MEMORY_UV_DATA[]={0,1,.5f,0,1.0f,1.0f};


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
    properties.rasterizationState.culling = RasterizationState::CullOptions::NONE;
    VertexDescription vertexDescription(2);
    vertexDescription.add("POSITION",GraphicsType::VECTOR2, 0,1, 0);
    vertexDescription.add("UV_COORDINATES",GraphicsType::VECTOR2, 0,1, 1);
    FrameBufferDescription frameBufferDescription;
    frameBufferDescription.colorTargets[0] = Pixels::Format::R8G8B8A8_UNORM;


    auto pipeline = GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(stages, 2, properties, vertexDescription, frameBufferDescription);
    auto finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    auto triangleData = std::unique_ptr<Buffer>(Buffer::newBuffer(DESCRIPTOR_MEMORY_TRIANGLE_DATA,sizeof(float)*6,Buffer::Accessibility::GPU,Buffer::UsageFlags::VERTEX_BUFFER));
    auto uvData = std::unique_ptr<Buffer>(Buffer::newBuffer(DESCRIPTOR_MEMORY_UV_DATA,sizeof(float)*6,Buffer::Accessibility::GPU,Buffer::UsageFlags::VERTEX_BUFFER));
    auto texture = utilities::loadTextureFromFile("resources/textures/gradient.jpg");
    auto target = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::RENDER_TARGET_ATTACHMENT,64,64,1,1,1));
    auto targetPixels = std::unique_ptr<Buffer>(Buffer::newBuffer(target->byteSize(Pixels::AspectFlags::COLOR),Buffer::Accessibility::CPU_AND_GPU));
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
    commandBuffer->bindGraphicsDescriptorGroup(0,samplerMemory.get(),0);
    commandBuffer->bindGraphicsDescriptorGroup(1,resourceMemory.get(),0);
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
            .texture = target.get(),
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
        .textureExtent = {target->width(),target->height(),1},
    };
    commandBuffer->copyTextureToBuffer(target.get(), targetPixels.get(),&mapping,1);
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

    GTEST_ASSERT_TRUE(utilities::matchesSimilarity(targetPixels.get(),"resources/textures/set-sampled-texture-result.png",.99,.9));
}
TEST(DescriptorMemory, SetStorageTexture)
{
    ShaderFile computeFile{
        .pathIndicator = "resources/shaders/ComputeDrawPush",
        .stage = ShaderStageFlags::COMPUTE,
    };
    auto pipeline = GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(computeFile);
    auto textureIndex = pipeline->descriptorGroup(0)->descriptorByteOffset(0);
    auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::COMPUTE));
    std::unique_ptr<ResourceDescriptorMemory> resourceMemory = std::unique_ptr<ResourceDescriptorMemory>(ResourceDescriptorMemory::newResourceDescriptorMemory(100));
    std::unique_ptr<SamplerDescriptorMemory> samplerMemory = std::unique_ptr<SamplerDescriptorMemory>(SamplerDescriptorMemory::newSamplerDescriptorMemory(100));
    auto texture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::STORAGE,256,256,1,1,1));
    auto buffer = std::unique_ptr<Buffer>(Buffer::newBuffer(texture->byteSize(Pixels::AspectFlags::COLOR),Buffer::Accessibility::CPU_AND_GPU));
    auto finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    commandBuffer->begin();
    commandBuffer->bindDescriptorMemory(resourceMemory.get(),samplerMemory.get());
    commandBuffer->bindComputeShaderPipeline(pipeline.get());
    resourceMemory->setStorageTexture(textureIndex,texture.get());
    commandBuffer->bindComputeDescriptorGroup(0,resourceMemory.get(),0);
    glm::vec4 color{1.0f,.1f,.1f,1.0f};
    commandBuffer->pushComputeConstants(0,sizeof(color),&color);
    commandBuffer->dispatch(texture->width()/pipeline->xComputeThreads(),texture->height()/pipeline->yComputeThreads(),1);
    commandBuffer->insertBarrier(TextureBarrier{.texture = texture.get(),.accessBefore = BarrierAccessFlags::SHADER_READ | BarrierAccessFlags::SHADER_WRITE, .accessAfter = BarrierAccessFlags::TRANSFER_READ, .syncBefore = PipelineStageFlags::COMPUTE_SHADER, .syncAfter = PipelineStageFlags::TRANSFER});
    TextureBufferMapping mapping
    {
        .bufferOffset = 0,
        .textureSubresource = {.aspectFlags = Pixels::AspectFlags::COLOR,.mipLevel = 0,.baseArrayLayer = 0,.layerCount = 1},
        .textureOffset = {0,0,0},
        .textureExtent = {texture->width(),texture->height(),1}
    };
    commandBuffer->copyTextureToBuffer(texture.get(),buffer.get(),&mapping,1);

    commandBuffer->end();
    SemaphoreValue signal{.semaphore = finished.get(),.value = 1};
    auto cmdPtr = commandBuffer.get();
    QueueSubmissionBatch submissionData
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = &cmdPtr,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1
    };
    slagGraphicsCard()->computeQueue()->submit(&submissionData,1);
    finished->waitForValue(1);
    GTEST_ASSERT_TRUE(utilities::matchesSimilarity(buffer.get(),"resources/textures/compute-draw-push.png",.99f,.9f));
}
TEST(DescriptorMemory, SetUniformTexelBuffer)
{
    ShaderFile stages[] =
    {
        {
            .pathIndicator = "resources/shaders/SetUniformTexelBuffer.vertex",
            .stage = ShaderStageFlags::VERTEX,
        },
        {
            .pathIndicator = "resources/shaders/SetUniformTexelBuffer.fragment",
            .stage = ShaderStageFlags::FRAGMENT,
        }
    };
    ShaderProperties properties{};
    VertexDescription vertexDescription(2);
    vertexDescription.add("POSITION",GraphicsType::VECTOR2, 0,1, 0);
    vertexDescription.add("UV_COORDINATES",GraphicsType::VECTOR2, 0,1, 1);
    FrameBufferDescription frameBufferDescription;
    frameBufferDescription.colorTargets[0] = Pixels::Format::R8G8B8A8_UNORM;


    auto pipeline = GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(stages, 2, properties, vertexDescription, frameBufferDescription);
    auto finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    auto triangleData = std::unique_ptr<Buffer>(Buffer::newBuffer(DESCRIPTOR_MEMORY_TRIANGLE_DATA,sizeof(float)*6,Buffer::Accessibility::GPU,Buffer::UsageFlags::VERTEX_BUFFER));
    auto uvData = std::unique_ptr<Buffer>(Buffer::newBuffer(DESCRIPTOR_MEMORY_UV_DATA,sizeof(float)*6,Buffer::Accessibility::GPU,Buffer::UsageFlags::VERTEX_BUFFER));
    auto textureBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(unsigned char)*8,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_TEXEL_BUFFER));
    auto target = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::RENDER_TARGET_ATTACHMENT,64,64,1,1,1));
    auto targetPixels = std::unique_ptr<Buffer>(Buffer::newBuffer(target->byteSize(Pixels::AspectFlags::COLOR),Buffer::Accessibility::CPU_AND_GPU));
    auto resourceMemory = std::unique_ptr<ResourceDescriptorMemory>(ResourceDescriptorMemory::newResourceDescriptorMemory(1000));
    auto samplerMemory = std::unique_ptr<SamplerDescriptorMemory>(SamplerDescriptorMemory::newSamplerDescriptorMemory(1000));
    auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::GRAPHICS));

    auto texelArray = textureBuffer->as<unsigned char>();
    texelArray[0] = 0;
    texelArray[1] = 255;
    texelArray[2] = 0;
    texelArray[3] = 255;
    texelArray[4] = 0;
    texelArray[5] = 0;
    texelArray[6] = 255;
    texelArray[7] = 255;


    resourceMemory->setUniformTexelBuffer(0+pipeline->descriptorGroup(0)->descriptorByteOffset(0),textureBuffer.get(),Pixels::Format::R8G8B8A8_UNORM,0,2);
    commandBuffer->begin();
    commandBuffer->setViewPort(0,0,target->width(),target->height(),0,1);
    commandBuffer->setScissors(slag::Rectangle{{0,0},{target->width(),target->height()}});
    Attachment attachment{.texture = target.get(),.autoClear=true,.clearValue=ClearColor{0,0,0,1.0f}};
    commandBuffer->beginRendering(&attachment,1,nullptr,slag::Rectangle{{0,0},{target->width(),target->height()}});
    commandBuffer->bindDescriptorMemory(resourceMemory.get(),samplerMemory.get());
    commandBuffer->bindGraphicsShaderPipeline(pipeline.get());
    commandBuffer->bindGraphicsDescriptorGroup(0,resourceMemory.get(),0);
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
            .texture = target.get(),
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
        .textureExtent = {target->width(),target->height(),1},
    };
    commandBuffer->copyTextureToBuffer(target.get(), targetPixels.get(),&mapping,1);
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


    GTEST_ASSERT_TRUE(utilities::matchesSimilarity(targetPixels.get(),"resources/textures/set-uniform-texel-buffer-result.png",.99,.9));
}
TEST(DescriptorMemory, SetStorageTexelBuffer)
{
    ShaderFile compute{
        .pathIndicator = "resources/shaders/SetStorageTexelBuffer",
        .stage = ShaderStageFlags::COMPUTE,
    };

    auto pipeline = GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(compute);
    auto resourceMemory = std::unique_ptr<ResourceDescriptorMemory>(ResourceDescriptorMemory::newResourceDescriptorMemory(1000));
    auto samplerMemory = std::unique_ptr<SamplerDescriptorMemory>(SamplerDescriptorMemory::newSamplerDescriptorMemory(1000));
    auto finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::COMPUTE));

    auto operands1 = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(float)*4*4,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::STORAGE_TEXEL_BUFFER));
    auto operands2 = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(unsigned char)*4*4,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::STORAGE_TEXEL_BUFFER));
    auto results = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(float)*4*4,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::STORAGE_TEXEL_BUFFER));

    auto operands1Array = operands1->as<float>();
    auto operands2Array = operands2->as<unsigned char>();
    operands1Array[0] = 0;
    operands1Array[1] = 0;
    operands1Array[2] = 0;
    operands1Array[3] = 1.0f;

    operands1Array[4] = 1.0f;
    operands1Array[5] = 0;
    operands1Array[6] = 0;
    operands1Array[7] = 1.0f;

    operands1Array[8] = 0;
    operands1Array[9] = 1.0f;
    operands1Array[10] = 0;
    operands1Array[11] = 1.0f;

    operands1Array[12] = 0;
    operands1Array[13] = 0;
    operands1Array[14] = 1.0f;
    operands1Array[15] = 1.0f;


    operands2Array[0] = 255;
    operands2Array[1] = 255;
    operands2Array[2] = 255;
    operands2Array[3] = 255;

    operands2Array[4] = 0;
    operands2Array[5] = 0;
    operands2Array[6] = 255;
    operands2Array[7] = 255;

    operands2Array[8] = 255;
    operands2Array[9] = 0;
    operands2Array[10] = 0;
    operands2Array[11] = 255;

    operands2Array[12] = 0;
    operands2Array[13] = 255;
    operands2Array[14] = 0;
    operands2Array[15] = 255;



    commandBuffer->begin();
    commandBuffer->bindDescriptorMemory(resourceMemory.get(),samplerMemory.get());
    resourceMemory->setStorageTexelBuffer(pipeline->descriptorGroup(0)->descriptorByteOffset(0),operands1.get(),Pixels::Format::R32G32B32A32_FLOAT,0,4);
    resourceMemory->setStorageTexelBuffer(pipeline->descriptorGroup(0)->descriptorByteOffset(1),operands2.get(),Pixels::Format::R8G8B8A8_UNORM,0,4);
    resourceMemory->setStorageTexelBuffer(pipeline->descriptorGroup(0)->descriptorByteOffset(2),results.get(),Pixels::Format::R32G32B32A32_FLOAT,0,4);
    commandBuffer->bindComputeShaderPipeline(pipeline.get());
    commandBuffer->bindComputeDescriptorGroup(0,resourceMemory.get(),0);
    commandBuffer->dispatch(4,1,1);
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
    slagGraphicsCard()->computeQueue()->submit(&submissionBatch,1);
    finished->waitForValue(1);
    auto resultPointer = results->as<glm::vec4>();
    GTEST_ASSERT_EQ(resultPointer[0],glm::vec4(.5f,.5f,.5f,1.0f));
    GTEST_ASSERT_EQ(resultPointer[1],glm::vec4(.5f,0.0f,.5f,1.0f));
    GTEST_ASSERT_EQ(resultPointer[2],glm::vec4(.5f,.5f,0.0f,1.0f));
    GTEST_ASSERT_EQ(resultPointer[3],glm::vec4(0.0f,.5f,.5f,1.0f));
}
TEST(DescriptorMemory, SetUniformBuffer)
{
    ShaderFile stages[] =
    {
        {
            .pathIndicator = "resources/shaders/SetUniformBuffer.vertex",
            .stage = ShaderStageFlags::VERTEX,
        },
        {
            .pathIndicator = "resources/shaders/SetUniformBuffer.fragment",
            .stage = ShaderStageFlags::FRAGMENT,
        }
    };
    ShaderProperties properties{};
    properties.rasterizationState.culling = RasterizationState::CullOptions::NONE;
    VertexDescription vertexDescription(2);
    vertexDescription.add("POSITION",GraphicsType::VECTOR2, 0,1, 0);
    vertexDescription.add("UV_COORDINATES",GraphicsType::VECTOR2, 0,1, 1);
    FrameBufferDescription frameBufferDescription;
    frameBufferDescription.colorTargets[0] = Pixels::Format::R8G8B8A8_UNORM;


    auto pipeline = GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(stages, 2, properties, vertexDescription, frameBufferDescription);
    auto finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    auto triangleData = std::unique_ptr<Buffer>(Buffer::newBuffer(DESCRIPTOR_MEMORY_TRIANGLE_DATA,sizeof(float)*6,Buffer::Accessibility::GPU,Buffer::UsageFlags::VERTEX_BUFFER));
    auto uvData = std::unique_ptr<Buffer>(Buffer::newBuffer(DESCRIPTOR_MEMORY_UV_DATA,sizeof(float)*6,Buffer::Accessibility::GPU,Buffer::UsageFlags::VERTEX_BUFFER));
    auto texture = utilities::loadTextureFromFile("resources/textures/gradient.jpg");
    auto target = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::RENDER_TARGET_ATTACHMENT,64,64,1,1,1));
    struct shaderParameters
    {
        glm::vec4 color1,color2;
        float boundary;
    };
    auto parameters = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(shaderParameters),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_BUFFER));
    auto shaderParams = parameters->as<shaderParameters>();
    shaderParams->color1 = glm::vec4(1.0f,0.0f,0.0f,1.0f);
    shaderParams->color2 = glm::vec4(0.0f,1.0f,0.0f,1.0f);
    shaderParams->boundary = .6f;
    auto targetPixels = std::unique_ptr<Buffer>(Buffer::newBuffer(target->byteSize(Pixels::AspectFlags::COLOR),Buffer::Accessibility::CPU_AND_GPU));
    auto resourceMemory = std::unique_ptr<ResourceDescriptorMemory>(ResourceDescriptorMemory::newResourceDescriptorMemory(1000));
    auto samplerMemory = std::unique_ptr<SamplerDescriptorMemory>(SamplerDescriptorMemory::newSamplerDescriptorMemory(1000));
    auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::GRAPHICS));

    resourceMemory->setUniformBuffer(0+pipeline->descriptorGroup(0)->descriptorByteOffset(0),parameters.get(),0,parameters->size());
    commandBuffer->begin();
    commandBuffer->setViewPort(0,0,target->width(),target->height(),0,1);
    commandBuffer->setScissors(slag::Rectangle{{0,0},{target->width(),target->height()}});
    Attachment attachment{.texture = target.get(),.autoClear=true,.clearValue=ClearColor{0,0,0,1.0f}};
    commandBuffer->beginRendering(&attachment,1,nullptr,slag::Rectangle{{0,0},{target->width(),target->height()}});
    commandBuffer->bindDescriptorMemory(resourceMemory.get(),samplerMemory.get());
    commandBuffer->bindGraphicsShaderPipeline(pipeline.get());
    commandBuffer->bindGraphicsDescriptorGroup(0,resourceMemory.get(),0);
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
            .texture = target.get(),
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
        .textureExtent = {target->width(),target->height(),1},
    };
    commandBuffer->copyTextureToBuffer(target.get(), targetPixels.get(),&mapping,1);
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

    GTEST_ASSERT_TRUE(utilities::matchesSimilarity(targetPixels.get(),"resources/textures/set-uniform-buffer-result.png",.99,.9));
}
TEST(DescriptorMemory, SetStorageBuffer)
{
    ShaderFile file{.pathIndicator = "resources/shaders/ParallelAdd", .stage = ShaderStageFlags::COMPUTE};
    auto compute = GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(file);
    std::unique_ptr<ResourceDescriptorMemory> resourceMemory = std::unique_ptr<ResourceDescriptorMemory>(ResourceDescriptorMemory::newResourceDescriptorMemory(100));
    std::unique_ptr<SamplerDescriptorMemory> samplerMemory = std::unique_ptr<SamplerDescriptorMemory>(SamplerDescriptorMemory::newSamplerDescriptorMemory(100));
    std::vector<float>inputs(128);
    for (int i=0; i<128; i++)
    {
        inputs[i] = i;
    }
    std::unique_ptr<Buffer> inputsBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(inputs.data(),128*sizeof(float),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::STORAGE_BUFFER));
    std::unique_ptr<Buffer> outputBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(64*sizeof(float),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::STORAGE_BUFFER));
    std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::COMPUTE));
    std::unique_ptr<Semaphore> finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));

    auto buffer0Index = compute->descriptorGroup(0)->descriptorByteOffset(0);
    auto buffer1Index = compute->descriptorGroup(0)->descriptorByteOffset(1);
    auto resultIndex = compute->descriptorGroup(0)->descriptorByteOffset(2);

    commandBuffer->begin();
    commandBuffer->bindDescriptorMemory(resourceMemory.get(),samplerMemory.get());

    resourceMemory->setStorageBuffer(buffer0Index,inputsBuffer.get(),sizeof(float),0, 64);
    resourceMemory->setStorageBuffer(buffer1Index,inputsBuffer.get(),sizeof(float),64, 64);
    resourceMemory->setStorageBuffer(resultIndex,outputBuffer.get(),sizeof(float),0, 64);
    commandBuffer->bindComputeShaderPipeline(compute.get());
    commandBuffer->bindComputeDescriptorGroup(0,resourceMemory.get(),0);

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
TEST(DescriptorMemory, SetSampler)
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
    properties.rasterizationState.culling = RasterizationState::CullOptions::NONE;
    VertexDescription vertexDescription(2);
    vertexDescription.add("POSITION",GraphicsType::VECTOR2, 0,1, 0);
    vertexDescription.add("UV_COORDINATES",GraphicsType::VECTOR2, 0,1, 1);
    FrameBufferDescription frameBufferDescription;
    frameBufferDescription.colorTargets[0] = Pixels::Format::R8G8B8A8_UNORM;


    auto pipeline = GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(stages, 2, properties, vertexDescription, frameBufferDescription);
    auto finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    auto triangleData = std::unique_ptr<Buffer>(Buffer::newBuffer(DESCRIPTOR_MEMORY_TRIANGLE_DATA,sizeof(float)*6,Buffer::Accessibility::GPU,Buffer::UsageFlags::VERTEX_BUFFER));
    auto uvData = std::unique_ptr<Buffer>(Buffer::newBuffer(DESCRIPTOR_MEMORY_UV_DATA,sizeof(float)*6,Buffer::Accessibility::GPU,Buffer::UsageFlags::VERTEX_BUFFER));
    auto texture = utilities::loadTextureFromFile("resources/textures/gradient.jpg");
    auto target = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::RENDER_TARGET_ATTACHMENT,64,64,1,1,1));
    auto targetPixels = std::unique_ptr<Buffer>(Buffer::newBuffer(target->byteSize(Pixels::AspectFlags::COLOR),Buffer::Accessibility::CPU_AND_GPU));
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
    commandBuffer->bindGraphicsDescriptorGroup(0,samplerMemory.get(),0);
    commandBuffer->bindGraphicsDescriptorGroup(1,resourceMemory.get(),0);
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
            .texture = target.get(),
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
        .textureExtent = {target->width(),target->height(),1},
    };
    commandBuffer->copyTextureToBuffer(target.get(), targetPixels.get(),&mapping,1);
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

    GTEST_ASSERT_TRUE(utilities::matchesSimilarity(targetPixels.get(),"resources/textures/set-sampled-texture-result.png",.99,.9));
}
#ifdef SLAG_DEBUG
TEST(DescriptorMemory, SetSampledTextureWrongTextureUsage)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    auto texture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::STORAGE,64,64,1,1,1));
    auto resourceMemory = std::unique_ptr<ResourceDescriptorMemory>(ResourceDescriptorMemory::newResourceDescriptorMemory(1000));
    EXPECT_DEATH(resourceMemory->setSampledTexture(0,texture.get()),"Given texture is not a sampled texture");
}
TEST(DescriptorMemory, SetStorageTextureFailWrongTextureUsage)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    auto texture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::SAMPLED_IMAGE,64,64,1,1,1));
    auto resourceMemory = std::unique_ptr<ResourceDescriptorMemory>(ResourceDescriptorMemory::newResourceDescriptorMemory(1000));
    EXPECT_DEATH(resourceMemory->setStorageTexture(0,texture.get()),"Given texture is not a storage texture");
}
TEST(DescriptorMemory, SetUniformTexelBufferWrongBufferUsage)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    auto buffer = std::unique_ptr<Buffer>(Buffer::newBuffer(256,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_BUFFER));
    auto resourceMemory = std::unique_ptr<ResourceDescriptorMemory>(ResourceDescriptorMemory::newResourceDescriptorMemory(1000));
    EXPECT_DEATH(resourceMemory->setUniformTexelBuffer(0,buffer.get(),Pixels::Format::R8G8B8A8_UNORM,0,8),"Given buffer is not a uniform texel buffer");
}
TEST(DescriptorMemory, SetStorageTexelBufferWrongBufferUsage)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    auto buffer = std::unique_ptr<Buffer>(Buffer::newBuffer(256,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_BUFFER));
    auto resourceMemory = std::unique_ptr<ResourceDescriptorMemory>(ResourceDescriptorMemory::newResourceDescriptorMemory(1000));
    EXPECT_DEATH(resourceMemory->setStorageTexelBuffer(0,buffer.get(),Pixels::Format::R8G8B8A8_UNORM,0,8),"Given buffer is not a storage texel buffer");
}
TEST(DescriptorMemory, SetUniformBufferWrongBufferUsage)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    auto buffer = std::unique_ptr<Buffer>(Buffer::newBuffer(256,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::STORAGE_BUFFER));
    auto resourceMemory = std::unique_ptr<ResourceDescriptorMemory>(ResourceDescriptorMemory::newResourceDescriptorMemory(1000));
    EXPECT_DEATH(resourceMemory->setUniformBuffer(0,buffer.get(),0,256),"Given buffer is not a uniform buffer");
}
TEST(DescriptorMemory, SetStorageBufferWrongBufferUsage)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    auto buffer = std::unique_ptr<Buffer>(Buffer::newBuffer(256,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_BUFFER));
    auto resourceMemory = std::unique_ptr<ResourceDescriptorMemory>(ResourceDescriptorMemory::newResourceDescriptorMemory(1000));
    EXPECT_DEATH(resourceMemory->setStorageBuffer(0,buffer.get(),64,0,4),"Given buffer is not a storage buffer");
}
TEST(DescriptorMemory, SetUniformTexelBufferLengthExceeded)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    auto buffer = std::unique_ptr<Buffer>(Buffer::newBuffer(256,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_TEXEL_BUFFER));
    auto resourceMemory = std::unique_ptr<ResourceDescriptorMemory>(ResourceDescriptorMemory::newResourceDescriptorMemory(1000));
    EXPECT_DEATH(resourceMemory->setUniformTexelBuffer(0,buffer.get(),Pixels::Format::R8G8B8A8_UNORM,0,65),"attempted to bind descriptor that exceeds buffer length");
}
TEST(DescriptorMemory, SetStorageTexelBufferBufferLengthExceeded)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    auto buffer = std::unique_ptr<Buffer>(Buffer::newBuffer(256,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::STORAGE_TEXEL_BUFFER));
    auto resourceMemory = std::unique_ptr<ResourceDescriptorMemory>(ResourceDescriptorMemory::newResourceDescriptorMemory(1000));
    EXPECT_DEATH(resourceMemory->setStorageTexelBuffer(0,buffer.get(),Pixels::Format::R8G8B8A8_UNORM,0,65),"attempted to bind descriptor that exceeds buffer length");
}
TEST(DescriptorMemory, SetUniformBufferBufferLengthExceeded)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    auto buffer = std::unique_ptr<Buffer>(Buffer::newBuffer(256,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_BUFFER));
    auto resourceMemory = std::unique_ptr<ResourceDescriptorMemory>(ResourceDescriptorMemory::newResourceDescriptorMemory(1000));
    EXPECT_DEATH(resourceMemory->setUniformBuffer(0,buffer.get(),0,512),"attempted to bind descriptor that exceeds buffer length");
}
TEST(DescriptorMemory, SetStorageBufferBufferLengthExceeded)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    auto buffer = std::unique_ptr<Buffer>(Buffer::newBuffer(256,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::STORAGE_BUFFER));
    auto resourceMemory = std::unique_ptr<ResourceDescriptorMemory>(ResourceDescriptorMemory::newResourceDescriptorMemory(1000));
    EXPECT_DEATH(resourceMemory->setStorageBuffer(0,buffer.get(),256,0,2),"attempted to bind descriptor that exceeds buffer length");
}
TEST(DescriptorMemory, SetUniformTexelBufferNonColorFormat)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    auto buffer = std::unique_ptr<Buffer>(Buffer::newBuffer(256,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_TEXEL_BUFFER));
    auto resourceMemory = std::unique_ptr<ResourceDescriptorMemory>(ResourceDescriptorMemory::newResourceDescriptorMemory(1000));
    EXPECT_DEATH(resourceMemory->setUniformTexelBuffer(0,buffer.get(),Pixels::Format::D32_FLOAT,0,8),"Only color formats can be bound as texel buffer");
}
TEST(DescriptorMemory, SetStorageTexelBufferNonColorFormat)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    auto buffer = std::unique_ptr<Buffer>(Buffer::newBuffer(256,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::STORAGE_TEXEL_BUFFER));
    auto resourceMemory = std::unique_ptr<ResourceDescriptorMemory>(ResourceDescriptorMemory::newResourceDescriptorMemory(1000));
    EXPECT_DEATH(resourceMemory->setStorageTexelBuffer(0,buffer.get(),Pixels::Format::D32_FLOAT,0,8),"Only color formats can be bound as texel buffer");
}
#endif
