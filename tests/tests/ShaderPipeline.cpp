#include <gtest/gtest.h>
#include <slag/Slag.h>
#include "../utilities/PlatformUtilities.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../third-party/LodePNG/lodepng.h"
#include "../utilities/SlagUtilities.h"

using namespace slag;

TEST(ShaderPipeline, Sandbox)
{
    auto card = Slag::backend()->graphicsCard(0);

    std::vector<glm::vec3> tverts = {{ -1.f, -1.f, 0.0f},{0.f,1.f, 0.0f},{1.f, -1.f, 0.0f}};
    std::vector<glm::vec2> tuvs = {{0,1},{.5,0},{1,1}};
    std::vector<glm::vec3> tnormals = {{0,0,1},{0,0,1},{0,0,1}};
    std::vector<uint16_t> tindexes = {0,1,2};

    auto triangleVerts = std::unique_ptr<Buffer>(card->newBuffer(tverts.size()*sizeof(glm::vec3),BufferCPUAccess::WRITE_ONLY));
    auto triangleUVs = std::unique_ptr<Buffer>(card->newBuffer(tuvs.size()*sizeof(glm::vec2),BufferCPUAccess::WRITE_ONLY));
    auto triangleNormals = std::unique_ptr<Buffer>(card->newBuffer(tnormals.size()*sizeof(glm::vec3),BufferCPUAccess::WRITE_ONLY));
    auto triangleIndices = std::unique_ptr<Buffer>(card->newBuffer(tindexes.size()*sizeof(uint16_t),BufferCPUAccess::WRITE_ONLY));

    auto tvertsPtr = triangleVerts->as<glm::vec3>();
    memcpy(tvertsPtr,tverts.data(),tverts.size()*sizeof(glm::vec3));
    auto tuvsPtr = triangleUVs->as<glm::vec2>();
    memcpy(tuvsPtr,tuvs.data(),tuvs.size()*sizeof(glm::vec2));
    auto tnormalsPtr = triangleNormals->as<glm::vec3>();
    memcpy(tnormalsPtr,tnormals.data(),tnormals.size()*sizeof(glm::vec3));
    auto tindicesPtr = triangleIndices->as<uint16_t>();
    memcpy(tindicesPtr,tindexes.data(),tindexes.size()*sizeof(uint16_t));

    auto colorTarget = std::unique_ptr<Texture>(card->newTexture2D(250,250,PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::COLOR_TARGET));
    auto depthTarget = std::unique_ptr<Texture>(card->newTexture2D(250,250,PixelFormat::D32_FLOAT,TextureUsageFlags::DEPTH_STENCIL_TARGET));

    auto globals = std::unique_ptr<Buffer>(card->newBuffer(/*192*/256,BufferCPUAccess::WRITE_ONLY,BufferMemoryType::UNIFORM));
    auto transform = std::unique_ptr<Buffer>(card->newBuffer(/*64*/256,BufferCPUAccess::WRITE_ONLY,BufferMemoryType::UNIFORM));
    auto proj = glm::perspective(95.0f,(float)colorTarget->width()/(float)colorTarget->height(),.01f,100.0f);
    glm::mat4 view = glm::mat4(1.0f);
    view = glm::translate(view,glm::vec3(0.0f,2.0f,5.0f));
    view = glm::rotate(view,glm::radians(-20.0f),glm::vec3(1.0f,0.0f,0.0f));
    view = glm::inverse(view);
    glm::mat4 projectionView = proj*view;
    auto globalsPtr = globals->as<glm::mat4>();
    globalsPtr[0] = proj;
    globalsPtr[1] = view;
    globalsPtr[2] = projectionView;

    auto transformPtr = transform->as<glm::mat4>();
    transformPtr[0] = glm::rotate(glm::mat4(1),glm::radians(45.0f),glm::vec3(0.0f,1.0f,0.0f));


    auto vertex = slagTestsCreateShaderModule(card, "resources/tests/shaders/compiled/TexturedDepth.vertex");
    auto fragment = slagTestsCreateShaderModule(card, "resources/tests/shaders/compiled/TexturedDepth.fragment");

    std::vector<VertexBinding> vertexBindings = {
        VertexBinding(0,sizeof(float)*3,std::vector<VertexAttribute>{VertexAttribute("POSITION",PixelFormat::R32G32B32_FLOAT,0)}),
        VertexBinding(1,sizeof(float)*2,std::vector<VertexAttribute>{VertexAttribute("UV_COORDINATES",PixelFormat::R32G32_FLOAT,0)}),
    };
    VertexDescription vertexDescription(vertexBindings);

    DescriptorRange constantRange(DescriptorRangeType::UNIFORM_BUFFER,0,0,1,0);
    DescriptorRange samplerRange(DescriptorRangeType::SAMPLER,1,0,1,0);
    DescriptorRange instanceRanges[]
    {
        DescriptorRange{DescriptorRangeType::UNIFORM_BUFFER,2,0,1,0},
        DescriptorRange{DescriptorRangeType::READONLY_RESOURCE,2,1,1,1}
    };
    PipelineInputMappingBuilder mappingBuilder(card);
    mappingBuilder.addDescriptorTable(&constantRange,1).addDescriptorTable(&samplerRange,1).addDescriptorTable(instanceRanges,2);
    auto bindings = mappingBuilder.build();
    FramebufferDescription framebufferDesc;
    framebufferDesc.colorFormats[0] = colorTarget->format();
    framebufferDesc.depthFormat = depthTarget->format();
    auto pipeline = std::unique_ptr<ShaderPipeline>(card->newShaderPipeline(vertexDescription,vertex.get(),fragment.get(),&bindings,PipelineState(),framebufferDesc));

    auto commandBuffer = std::unique_ptr<CommandBuffer>(card->newCommandBuffer(QueueType::GRAPHICS));
    auto finished = std::unique_ptr<Semaphore>(card->newSemaphore());

    auto sampler = std::unique_ptr<Sampler>(card->newSampler());
    auto texture = utilities::loadTexture("resources\\tests\\textures\\gradient.jpg",card);


    auto resourceHeap = std::unique_ptr<DescriptorHeap>(card->newDescriptorHeap(DescriptorHeapType::RESOURCE,card->descriptorHeapDetails().resourceDescriptorIncrementSize*500));
    auto samplerHeap = std::unique_ptr<DescriptorHeap>(card->newDescriptorHeap(DescriptorHeapType::SAMPLER,card->descriptorHeapDetails().samplerDescriptorIncrementSize*200));
    uint32_t resourceHeapOffset = 0;
    uint32_t samplerHeapOffset = 0;

    auto table0Offset = resourceHeapOffset;
    resourceHeap->writeUniformBufferDescriptor(resourceHeapOffset,globals.get(),0,globals->size());
    resourceHeapOffset += card->descriptorHeapDetails().resourceDescriptorIncrementSize;

    auto table1Offset = samplerHeapOffset;
    samplerHeap->writeSamplerDescriptor(samplerHeapOffset,sampler.get());
    samplerHeapOffset += card->descriptorHeapDetails().samplerDescriptorIncrementSize;

    auto table2Offset = resourceHeapOffset;
    resourceHeap->writeUniformBufferDescriptor(resourceHeapOffset,transform.get(),0,transform->size());
    resourceHeapOffset += card->descriptorHeapDetails().resourceDescriptorIncrementSize;
    resourceHeap->writeSampledTextureDescriptor(resourceHeapOffset,texture.get());
    resourceHeapOffset += card->descriptorHeapDetails().resourceDescriptorIncrementSize;


    commandBuffer->begin();

    commandBuffer->bindDescriptorHeaps(resourceHeap.get(),samplerHeap.get());
    commandBuffer->setViewPort(0,0,colorTarget->width(),colorTarget->height(),0.0f,1.0f);
    commandBuffer->setScissors(Rectangle{0,0,colorTarget->width(),colorTarget->height()});
    Attachment colorAttachment(colorTarget.get(),true,ClearValue{.5,.2,.1,1});
    Attachment depthAttachment(depthTarget.get(),true,ClearValue{1,0});
    commandBuffer->beginRendering(&colorAttachment,1,&depthAttachment,Rectangle{0,0,colorTarget->width(),colorTarget->height()});

    Buffer* buffers[] = {triangleVerts.get(),triangleUVs.get()};
    uint64_t offsets[] = {0,0};
    uint64_t strides[] = {sizeof(glm::vec3),sizeof(glm::vec2)};
    commandBuffer->bindVertexBuffers(0,buffers,offsets,strides,2);
    commandBuffer->bindIndexBuffer(triangleIndices.get(),IndexBufferType::UINT_16,0);
    commandBuffer->bindGraphicsPipeline(pipeline.get());

    commandBuffer->setInputBindingTable(bindings.input(0).location(),table0Offset);
    commandBuffer->setInputBindingTable(bindings.input(1).location(),table1Offset);
    commandBuffer->setInputBindingTable(bindings.input(2).location(),table2Offset);
    commandBuffer->drawIndexed(tindexes.size(),1,0,0,0);

    commandBuffer->endRendering();

    commandBuffer->end();
    auto cmdBufferPtr = commandBuffer.get();
    SemaphoreValue signal{.semaphore = finished.get(),.value =1};
    SubmissionBatch batch
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = &cmdBufferPtr,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1,
    };
    card->graphicsQueue()->submit(&batch,1);
    finished->waitForValue(1);
    utilities::saveTexture("C:\\Users\\jshelton\\Desktop\\results\\output.png",colorTarget.get());
    GTEST_FAIL();
}

TEST(ShaderPipeline, SandboxBindless)
{
    auto card = Slag::backend()->graphicsCard(0);

    std::vector<glm::vec3> tverts = {{ -1.f, -1.f, 0.0f},{0.f,1.f, 0.0f},{1.f, -1.f, 0.0f}};
    std::vector<glm::vec2> tuvs = {{0,1},{.5,0},{1,1}};
    std::vector<glm::vec3> tnormals = {{0,0,1},{0,0,1},{0,0,1}};
    std::vector<uint16_t> tindexes = {0,1,2};

    auto triangleVerts = std::unique_ptr<Buffer>(card->newBuffer(tverts.size()*sizeof(glm::vec3),BufferCPUAccess::WRITE_ONLY));
    auto triangleUVs = std::unique_ptr<Buffer>(card->newBuffer(tuvs.size()*sizeof(glm::vec2),BufferCPUAccess::WRITE_ONLY));
    auto triangleNormals = std::unique_ptr<Buffer>(card->newBuffer(tnormals.size()*sizeof(glm::vec3),BufferCPUAccess::WRITE_ONLY));
    auto triangleIndices = std::unique_ptr<Buffer>(card->newBuffer(tindexes.size()*sizeof(uint16_t),BufferCPUAccess::WRITE_ONLY));

    auto tvertsPtr = triangleVerts->as<glm::vec3>();
    memcpy(tvertsPtr,tverts.data(),tverts.size()*sizeof(glm::vec3));
    auto tuvsPtr = triangleUVs->as<glm::vec2>();
    memcpy(tuvsPtr,tuvs.data(),tuvs.size()*sizeof(glm::vec2));
    auto tnormalsPtr = triangleNormals->as<glm::vec3>();
    memcpy(tnormalsPtr,tnormals.data(),tnormals.size()*sizeof(glm::vec3));
    auto tindicesPtr = triangleIndices->as<uint16_t>();
    memcpy(tindicesPtr,tindexes.data(),tindexes.size()*sizeof(uint16_t));

    auto vertex = slagTestsCreateShaderModule(card, "resources/tests/shaders/compiled/TexturedDepthBindless.vertex");
    auto fragment = slagTestsCreateShaderModule(card, "resources/tests/shaders/compiled/TexturedDepthBindless.fragment");
    std::vector<VertexBinding> vertexBindings =
    {
        VertexBinding(0,sizeof(float)*3,std::vector<VertexAttribute>{VertexAttribute("POSITION",PixelFormat::R32G32B32_FLOAT,0)}),
        VertexBinding(1,sizeof(float)*2,std::vector<VertexAttribute>{VertexAttribute("UV_COORDINATES",PixelFormat::R32G32_FLOAT,0)})
    };
    GTEST_FAIL();
}

TEST(ShaderPipeline, OverlappingBindGroups)
{
    //Need to test a bind group where some / all descriptors are used on multiple stages
    GTEST_FAIL();
}

TEST(ShaderPipeline, IncompatibleBindGroups)
{
    GTEST_FAIL();
}