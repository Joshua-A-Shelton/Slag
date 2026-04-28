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


    auto vertex = slagTestsCreateShaderModule(card, "resources/tests/shaders/compiled/TexturedDepth.vertex");
    auto fragment = slagTestsCreateShaderModule(card, "resources/tests/shaders/compiled/TexturedDepth.fragment");
    std::vector<VertexBinding> vertexBindings =
    {
        VertexBinding(0,sizeof(float)*3,std::vector<VertexAttribute>{VertexAttribute("POSITION",PixelFormat::R32G32B32_FLOAT,0)}),
        VertexBinding(1,sizeof(float)*2,std::vector<VertexAttribute>{VertexAttribute("UV_COORDINATES",PixelFormat::R32G32_FLOAT,0)})
    };
    VertexDescription vertexDesc(vertexBindings);
    FramebufferDescription framebufferDesc{};
    framebufferDesc.colorFormats[0] = PixelFormat::R8G8B8A8_UNORM;
    framebufferDesc.depthFormat = PixelFormat::D32_FLOAT;
    PipelineInputMappingBuilder pipelineBindingBuilder(card);
    BufferLayout globalLayout(std::vector<StructMember>
    {
        StructMember("projection",GraphicsType::MATRIX_4X4,1,0,0),
        StructMember("view",GraphicsType::MATRIX_4X4,1,64,64),
        StructMember("projectionView",GraphicsType::MATRIX_4X4,1,128,128),
    });
    BufferLayout InstanceLayout(std::vector<StructMember>{StructMember("transform",GraphicsType::DOUBLE_MATRIX_4X4,1,0,0)});
    DescriptorRange globalsDescriptor{.type = DescriptorRangeType::UNIFORM_BUFFER,.bindGroupIndex = 0,.firstBinding = 0,.bindingCount = 1,.offsetInDescriptorsFromTableStart = 0};
    pipelineBindingBuilder.addDescriptorTable(&globalsDescriptor,1);
    DescriptorRange samplerDescriptor{.type = DescriptorRangeType::SAMPLER,.bindGroupIndex = 1,.firstBinding = 0,.bindingCount = 1, .offsetInDescriptorsFromTableStart = 0};
    pipelineBindingBuilder.addDescriptorTable(&samplerDescriptor,1);
    DescriptorRange instance[] =
    {
        {.type = DescriptorRangeType::UNIFORM_BUFFER,.bindGroupIndex = 2,.firstBinding = 0,.bindingCount = 1,.offsetInDescriptorsFromTableStart = 0},
        {.type = DescriptorRangeType::READONLY_RESOURCE,.bindGroupIndex = 2,.firstBinding = 1,.bindingCount = 1,.offsetInDescriptorsFromTableStart = 1}
    };
    pipelineBindingBuilder.addDescriptorTable(instance,2);
    auto bindings = pipelineBindingBuilder.build();
    auto pipeline = std::unique_ptr<ShaderPipeline>(card->newShaderPipeline(vertexDesc,vertex.get(), fragment.get(),&bindings,PipelineState{},framebufferDesc));
    auto commandBuffer = std::unique_ptr<CommandBuffer>(card->newCommandBuffer(QueueType::GRAPHICS));
    auto colorTarget = std::unique_ptr<Texture>(card->newTexture2D(250,250,PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::COLOR_TARGET));
    auto depthTarget = std::unique_ptr<Texture>(card->newTexture2D(250,250,PixelFormat::D32_FLOAT,TextureUsageFlags::DEPTH_STENCIL_TARGET));
    auto texture = utilities::loadTexture("resources\\tests\\textures\\gradient.jpg",card);
    auto resultBuffer = std::unique_ptr<Buffer>(card->newBuffer(colorTarget->bufferSize(PixelAspect::COLOR),BufferCPUAccess::READ_WRITE));
    auto finished = std::unique_ptr<Semaphore>(card->newSemaphore());
    auto resourceHeap = std::unique_ptr<DescriptorHeap>(card->newDescriptorHeap(DescriptorHeapType::RESOURCE,256000));
    auto samplerHeap = std::unique_ptr<DescriptorHeap>(card->newDescriptorHeap(DescriptorHeapType::SAMPLER,25600));
    auto sampler = std::unique_ptr<Sampler>(card->newSampler());
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

    commandBuffer->begin();
    commandBuffer->bindDescriptorHeaps(resourceHeap.get(),samplerHeap.get());
    commandBuffer->setViewPort(0,0,250,250,0,1);
    commandBuffer->setScissors(Rectangle{0,0,250,250});
    Attachment colorAttachment{.texture = colorTarget.get(),.autoClear = true, .clearValue = ClearValue{.color{1.0f,.5f,.5f,1.0f}}};
    Attachment depthAttachment{.texture = depthTarget.get(),.autoClear = true, .clearValue = ClearValue{.depthStencil = {.depth = 1, .stencil = 0}}};
    commandBuffer->bindGraphicsPipeline(pipeline.get());
    commandBuffer->beginRendering(&colorAttachment,1,&depthAttachment,Rectangle{0,0,250,250});
    uint32_t offset = 0;
    resourceHeap->writeUniformBufferDescriptor(bindings.input(0).location(),globals.get(),0,globals->size());
    commandBuffer->setInputBindingTable(0,offset);
    offset += bindings.input(2).location();
    samplerHeap->writeSamplerDescriptor(0,sampler.get());
    commandBuffer->setInputBindingTable(bindings.input(1).location(),0);
    resourceHeap->writeUniformBufferDescriptor(offset,transform.get(),0,transform->size());
    resourceHeap->writeSampledTextureDescriptor(offset+card->descriptorTableDetails().sampledTextureSize,texture.get());
    commandBuffer->setInputBindingTable(bindings.input(2).location(),offset);
    //offset+=bindings.descriptorTable(2).size();

    commandBuffer->bindIndexBuffer(triangleIndices.get(),IndexBufferType::UINT_16,0);
    Buffer* vertexBuffers[] {triangleVerts.get(),triangleUVs.get()};
    uint64_t offsets[]{0,0};
    uint64_t strides[]
    {
        sizeof(glm::vec3),
        sizeof(glm::vec2)
    };
    commandBuffer->bindVertexBuffers(0,vertexBuffers,offsets,strides,2);
    commandBuffer->drawIndexed(3,1,0,0,0);

    commandBuffer->endRendering();
    TextureBarrier finishedRendering
    {
        .texture = colorTarget.get(),
        .baseLayer = 0,
        .layerCount = 1,
        .baseMipLevel = 0,
        .mipCount = 1,
        .syncBefore = SyncStages::COLOR_TARGET_OUTPUT,
        .syncAfter = SyncStages::COPY,
        .flush = MemoryCaches::COLOR_TARGET,
        .invalidate = MemoryCaches::COPY_READ,
    };
    commandBuffer->insertBarriers(&finishedRendering,1);
    TextureBufferMapping mapping
    {
        .bufferOffset = 0,
        .subresource =
            {
                .aspect = PixelAspect::COLOR,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
        .offset = {0,0,0},
        .extent = {colorTarget->width(),colorTarget->height(),1}
    };
    commandBuffer->copyTextureToBuffer(colorTarget.get(),resultBuffer.get(),&mapping,1);
    commandBuffer->end();

    auto rawCB = commandBuffer.get();
    SemaphoreValue signal{.semaphore = finished.get(),.value = 1};
    SubmissionBatch submissionBatch
    {
        .commandBuffers = &rawCB,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1
    };
    card->graphicsQueue()->submit(&submissionBatch,1);
    finished->waitForValue(1);
    lodepng::encode("C:\\Users\\jshelton\\Desktop\\results\\sandbox.png",resultBuffer->as<unsigned char>(),colorTarget->width(),colorTarget->height());
    int i=0;
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