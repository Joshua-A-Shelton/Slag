#include <gtest/gtest.h>
#include <slag/Slag.h>
#include "../GraphicsAPIEnvironment.h"
#include "../Utilities.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../third-party/LodePNG/lodepng.h"

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
using namespace slag;
//Some compilers (like slangc) may treat matrices as structs of 4 vector4s, we'll allow it
bool is4x4MatrixType(const UniformBufferDescriptorLayout* layout)
{
    if (layout->type()==GraphicsType::DOUBLE_MATRIX_4X4)
    {
        return true;
    }
    else if (layout->type()==GraphicsType::STRUCT && layout->size() == 64)
    {
        if (layout->childrenCount() == 1)
        {
            auto& child = layout->operator[](0);
            if (child.type() == GraphicsType::VECTOR4 && child.arrayDepth()==4)
            {
                return true;
            }
        }
        else if (layout->childrenCount()==4)
        {
            bool uniformChildren = true;
            for (auto i=0; i < layout->childrenCount(); i++)
            {
                if (layout[i].type() != GraphicsType::VECTOR4)
                {
                    uniformChildren = false;
                }
            }
            return uniformChildren;
        }
    }
    return false;
}

class ShaderPipelineTest: public ::testing::Test
{
protected:
    std::unique_ptr<Buffer> triangleVerts;
    std::unique_ptr<Buffer> triangleUVs;
    std::unique_ptr<Buffer> triangleIndicies;
    VertexDescription vertexPosUVDescription = VertexDescription(2);
    std::unique_ptr<Texture> object1Texture;
    std::unique_ptr<Texture> object2Texture;
    std::unique_ptr<Sampler> defaultSampler;
    uint32_t imageSize = 150;

    void testProperties(ShaderProperties properties1, ShaderProperties properties2,glm::mat4 cameraTransform, glm::mat4 cameraProjection, glm::mat4 object1Transform, glm::mat4 object2Transform, const std::filesystem::path& compareResult, float overallSimilarityScore, float individualPixelScore)
    {
        std::unique_ptr<Buffer> globalsBuffer;
        std::unique_ptr<Buffer> objectBuffer;

        globalsBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(GlobalSet0Group),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_BUFFER));
        auto globalDataPtr = globalsBuffer->as<GlobalSet0Group>();
        glm::mat4 proj = cameraProjection;
        glm::mat4 view = glm::inverse(cameraTransform);
        glm::mat4 projectionView = proj*view;
        globalDataPtr->projection = proj;
        globalDataPtr->view = view;
        globalDataPtr->projectionView = projectionView;

        objectBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(TexturedDepthSet1Group)*2,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_BUFFER));
        auto objectsDataPtr = objectBuffer->as<TexturedDepthSet1Group>();
        objectsDataPtr[0].position = object1Transform;
        objectsDataPtr[1].position = object2Transform;


        auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::GRAPHICS));
        ShaderFile files[]=
        {
            ShaderFile("resources/shaders/TexturedDepth.vertex",ShaderStageFlags::VERTEX),
            ShaderFile("resources/shaders/TexturedDepth.fragment",ShaderStageFlags::FRAGMENT)
        };
        FrameBufferDescription framebufferDescription;
        framebufferDescription.colorTargets[0] = Pixels::Format::R8G8B8A8_UNORM;
        framebufferDescription.depthTarget = Pixels::Format::D24_UNORM_S8_UINT;
        auto shader1 = GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(files,2,properties1,vertexPosUVDescription,framebufferDescription);
        auto shader2 = GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(files,2,properties2,vertexPosUVDescription,framebufferDescription);
        auto descriptorPool = std::unique_ptr<DescriptorPool>(DescriptorPool::newDescriptorPool());
        auto target = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::RENDER_TARGET_ATTACHMENT,imageSize,imageSize,1,1));
        auto depth = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::D24_UNORM_S8_UINT,Texture::Type::TEXTURE_2D,Texture::UsageFlags::DEPTH_STENCIL_ATTACHMENT,imageSize,imageSize,1,1));
        auto targetOutput = std::unique_ptr<Buffer>(Buffer::newBuffer(target->byteSize(),Buffer::Accessibility::CPU_AND_GPU));

        auto finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));

        descriptorPool->reset();
        auto globalsData = descriptorPool->makeBundle(shader1->descriptorGroup(0));
        auto object1Data = descriptorPool->makeBundle(shader1->descriptorGroup(1));
        auto object2Data = descriptorPool->makeBundle(shader2->descriptorGroup(1));

        globalsData.setUniformBuffer(0,0,globalsBuffer.get(),0,sizeof(GlobalSet0Group));
        object1Data.setUniformBuffer(0,0,objectBuffer.get(),0,sizeof(TexturedDepthSet1Group));
        object1Data.setTextureAndSampler(1,0,object1Texture.get(),defaultSampler.get());
        object2Data.setUniformBuffer(0,0,objectBuffer.get(),sizeof(TexturedDepthSet1Group),sizeof(TexturedDepthSet1Group));
        object2Data.setTextureAndSampler(1,0,object2Texture.get(),defaultSampler.get());


        commandBuffer->begin();

        commandBuffer->bindDescriptorPool(descriptorPool.get());
        commandBuffer->setViewPort(0,0,imageSize,imageSize,0,1);
        commandBuffer->setScissors(slag::Rectangle{.offset = {0,0}, .extent = {imageSize,imageSize}});
        commandBuffer->bindGraphicsShaderPipeline(shader1.get());
        Attachment colorAttachment{.texture = target.get(),.autoClear = true,.clearValue = ClearValue{.color = {.floats = {.7,.3,.1,1}}}};
        Attachment depthAttachment{.texture = depth.get(),.autoClear = true, .clearValue = ClearValue{.depthStencil = {.depth = 1,.stencil = 0}}};

        commandBuffer->beginRendering(&colorAttachment,1,&depthAttachment,slag::Rectangle{.offset = {0,0}, .extent = {imageSize,imageSize}});
        commandBuffer->bindIndexBuffer(triangleIndicies.get(),Buffer::IndexSize::UINT16,0);
        Buffer* buffers[]
        {
            triangleVerts.get(),
            triangleUVs.get(),
        };
        uint64_t offsets[]
        {
            0,0
        };
        uint64_t strides[]
        {
            sizeof(glm::vec3),
            sizeof(glm::vec2),
        };
        commandBuffer->bindVertexBuffers(0,buffers,offsets,strides,2);
        commandBuffer->bindGraphicsDescriptorBundle(0,globalsData);
        commandBuffer->bindGraphicsDescriptorBundle(1,object1Data);
        commandBuffer->drawIndexed(triangleVerts->countAsArray<glm::vec3>(),1,0,0,0);
        commandBuffer->bindGraphicsShaderPipeline(shader2.get());
        commandBuffer->bindGraphicsDescriptorBundle(1,object2Data);
        commandBuffer->drawIndexed(triangleVerts->countAsArray<glm::vec3>(),1,0,0,0);

        commandBuffer->endRendering();

        commandBuffer->insertBarrier(TextureBarrier{.texture = target.get(),.baseLayer = 0,.layerCount = 1,.baseMipLevel = 0,.mipCount = 1,.accessBefore = BarrierAccessFlags::SHADER_WRITE,.accessAfter = BarrierAccessFlags::TRANSFER_READ,.syncBefore = PipelineStageFlags::ALL_GRAPHICS,.syncAfter = PipelineStageFlags::TRANSFER});

        TextureToBufferCopyData copyData{.bufferOffset = 0, .subresource = TextureSubresource{Pixels::AspectFlags::COLOR,0,0,1}};
        commandBuffer->copyTextureToBuffer(target.get(),&copyData,1,targetOutput.get());

        commandBuffer->end();

        auto bufferLocation = commandBuffer.get();
        SemaphoreValue signal
        {
            .semaphore = finished.get(),
            .value = 1
        };
        QueueSubmissionBatch submissionBatch{.waitSemaphores = nullptr,.waitSemaphoreCount = 0,.commandBuffers = &bufferLocation,.commandBufferCount = 1,.signalSemaphores = &signal,.signalSemaphoreCount= 1};
        slagGraphicsCard()->graphicsQueue()->submit(&submissionBatch,1);
        finished->waitForValue(1);


        auto pixels = targetOutput->as<uint8_t>();

        if (!std::filesystem::exists(compareResult))
        {
            GTEST_FAIL();
            return;
        }

        auto groundTruth = utilities::loadTexelsFromFile(compareResult);

        GTEST_ASSERT_EQ(groundTruth.size(),targetOutput->countAsArray<uint8_t>());
        float maxDifference = 255.0f*(1-individualPixelScore);
        std::vector<float> pixelSimilarity(groundTruth.size()/4);
        if (maxDifference > 0)
        {
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

                if (individualPixelScore > 0)
                {
                    GTEST_ASSERT_LE(pixelDifference,maxDifference);
                }
                float percentSimilar = (255.0f-pixelDifference)/255.0f;
                GTEST_ASSERT_GE(percentSimilar,individualPixelScore);
                pixelSimilarity[i/4] = percentSimilar;
            }
            float total = 0;
            for (int i=0; i< pixelSimilarity.size(); i++)
            {
                total += pixelSimilarity[i];
            }
            float overallSimilarity = total/pixelSimilarity.size();
            GTEST_ASSERT_GE(overallSimilarity,overallSimilarityScore);
        }
        else
        {
            for (int i=0; i<groundTruth.size(); i++)
            {
                GTEST_ASSERT_EQ(groundTruth[i],pixels[i]);
            }
        }

    }
    void testPropertiesMultiSample(ShaderProperties properties,glm::mat4 cameraTransform, glm::mat4 cameraProjection, glm::mat4 object1Transform, glm::mat4 object2Transform, const std::filesystem::path& compareResult, float overallSimilarityScore, float individualPixelScore)
    {
        auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::GRAPHICS));
        auto finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore());
        auto target = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::RENDER_TARGET_ATTACHMENT,150,150,1,1,Texture::SampleCount::EIGHT));
        auto depth = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::D32_FLOAT,Texture::Type::TEXTURE_2D,Texture::UsageFlags::DEPTH_STENCIL_ATTACHMENT,150,150,1,1,Texture::SampleCount::EIGHT));
        auto final = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::RENDER_TARGET_ATTACHMENT,150,150,1,1));

        ShaderFile files[]=
          {
            ShaderFile("resources/shaders/TexturedDepth.vertex",ShaderStageFlags::VERTEX),
            ShaderFile("resources/shaders/TexturedDepth.fragment",ShaderStageFlags::FRAGMENT)
        };

        FrameBufferDescription framebufferDescription;
        framebufferDescription.colorTargets[0] = Pixels::Format::R8G8B8A8_UNORM;
        framebufferDescription.depthTarget = Pixels::Format::D32_FLOAT;

        auto shader = GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(files,2,properties,vertexPosUVDescription,framebufferDescription);

        std::unique_ptr<Buffer> globalsBuffer;
        std::unique_ptr<Buffer> objectBuffer;

        globalsBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(GlobalSet0Group),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_BUFFER));
        auto globalDataPtr = globalsBuffer->as<GlobalSet0Group>();
        glm::mat4 proj = cameraProjection;
        glm::mat4 view = glm::inverse(cameraTransform);
        glm::mat4 projectionView = proj*view;
        globalDataPtr->projection = proj;
        globalDataPtr->view = view;
        globalDataPtr->projectionView = projectionView;

        objectBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(TexturedDepthSet1Group)*2,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_BUFFER));
        auto objectsDataPtr = objectBuffer->as<TexturedDepthSet1Group>();
        objectsDataPtr[0].position = object1Transform;
        objectsDataPtr[1].position = object2Transform;



        auto descriptorPool = std::unique_ptr<DescriptorPool>(DescriptorPool::newDescriptorPool());
        auto targetOutput = std::unique_ptr<Buffer>(Buffer::newBuffer(final->byteSize(),Buffer::Accessibility::CPU_AND_GPU));


        descriptorPool->reset();
        auto globalsData = descriptorPool->makeBundle(shader->descriptorGroup(0));
        auto object1Data = descriptorPool->makeBundle(shader->descriptorGroup(1));
        auto object2Data = descriptorPool->makeBundle(shader->descriptorGroup(1));

        globalsData.setUniformBuffer(0,0,globalsBuffer.get(),0,sizeof(GlobalSet0Group));
        object1Data.setUniformBuffer(0,0,objectBuffer.get(),0,sizeof(TexturedDepthSet1Group));
        object1Data.setTextureAndSampler(1,0,object1Texture.get(),defaultSampler.get());
        object2Data.setUniformBuffer(0,0,objectBuffer.get(),sizeof(TexturedDepthSet1Group),sizeof(TexturedDepthSet1Group));
        object2Data.setTextureAndSampler(1,0,object2Texture.get(),defaultSampler.get());



        commandBuffer->begin();

        commandBuffer->bindDescriptorPool(descriptorPool.get());
        commandBuffer->setViewPort(0,0,imageSize,imageSize,0,1);
        commandBuffer->setScissors(slag::Rectangle{.offset = {0,0}, .extent = {imageSize,imageSize}});
        commandBuffer->bindGraphicsShaderPipeline(shader.get());
        Attachment colorAttachment{.texture = target.get(),.autoClear = true,.clearValue = ClearValue{.color = {.floats = {.7,.3,.1,1}}}};
        Attachment depthAttachment{.texture = depth.get(),.autoClear = true, .clearValue = ClearValue{.depthStencil = {.depth = 1,.stencil = 0}}};

        commandBuffer->beginRendering(&colorAttachment,1,&depthAttachment,slag::Rectangle{.offset = {0,0}, .extent = {imageSize,imageSize}});
        commandBuffer->bindIndexBuffer(triangleIndicies.get(),Buffer::IndexSize::UINT16,0);
        Buffer* buffers[]
        {
            triangleVerts.get(),
            triangleUVs.get(),
        };
        uint64_t offsets[]
        {
            0,0
        };
        uint64_t strides[]
        {
            sizeof(glm::vec3),
            sizeof(glm::vec2),
        };
        commandBuffer->bindVertexBuffers(0,buffers,offsets,strides,2);
        commandBuffer->bindGraphicsDescriptorBundle(0,globalsData);
        commandBuffer->bindGraphicsDescriptorBundle(1,object1Data);
        commandBuffer->drawIndexed(triangleVerts->countAsArray<glm::vec3>(),1,0,0,0);
        commandBuffer->bindGraphicsDescriptorBundle(1,object2Data);
        commandBuffer->drawIndexed(triangleVerts->countAsArray<glm::vec3>(),1,0,0,0);

        commandBuffer->endRendering();

        commandBuffer->insertBarrier(TextureBarrier{.texture = target.get(),.baseLayer = 0,.layerCount = 1,.baseMipLevel = 0,.mipCount = 1,.accessBefore = BarrierAccessFlags::SHADER_WRITE,.accessAfter = BarrierAccessFlags::BLIT_READ,.syncBefore = PipelineStageFlags::ALL_GRAPHICS,.syncAfter = PipelineStageFlags::BLIT});

        commandBuffer->resolve(target.get(),0,0,{0,0},final.get(),0,0,{0,0},{150,150});

        commandBuffer->insertBarrier(TextureBarrier{.texture = final.get(),.baseLayer = 0,.layerCount = 1,.baseMipLevel = 0,.mipCount = 1,.accessBefore = BarrierAccessFlags::BLIT_WRITE,.accessAfter = BarrierAccessFlags::TRANSFER_READ,.syncBefore = PipelineStageFlags::BLIT,.syncAfter = PipelineStageFlags::TRANSFER});



        TextureToBufferCopyData copyData{.bufferOffset = 0, .subresource = TextureSubresource{Pixels::AspectFlags::COLOR,0,0,1}};
        commandBuffer->copyTextureToBuffer(final.get(),&copyData,1,targetOutput.get());

        commandBuffer->end();

        SemaphoreValue signal{finished.get(),1};
        auto buffer = commandBuffer.get();

        QueueSubmissionBatch submissionBatch{};
        submissionBatch.waitSemaphores = nullptr;
        submissionBatch.waitSemaphoreCount = 0;
        submissionBatch.commandBufferCount =1;
        submissionBatch.commandBuffers = &buffer;
        submissionBatch.signalSemaphores = &signal;
        submissionBatch.signalSemaphoreCount = 1;

        slagGraphicsCard()->graphicsQueue()->submit(&submissionBatch,1);

        finished->waitForValue(1);

        auto pixels = targetOutput->as<uint8_t>();

        if (!std::filesystem::exists(compareResult))
        {
            GTEST_FAIL();
            return;
        }

        auto groundTruth = utilities::loadTexelsFromFile(compareResult);

        GTEST_ASSERT_EQ(groundTruth.size(),targetOutput->countAsArray<uint8_t>());
        float maxDifference = 255.0f*(1-individualPixelScore);
        std::vector<float> pixelSimilarity(groundTruth.size()/4);
        if (maxDifference > 0)
        {
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

                if (individualPixelScore > 0)
                {
                    GTEST_ASSERT_LE(pixelDifference,maxDifference);
                }
                float percentSimilar = (255.0f-pixelDifference)/255.0f;
                GTEST_ASSERT_GE(percentSimilar,individualPixelScore);
                pixelSimilarity[i/4] = percentSimilar;
            }
            float total = 0;
            for (int i=0; i< pixelSimilarity.size(); i++)
            {
                total += pixelSimilarity[i];
            }
            float overallSimilarity = total/pixelSimilarity.size();
            GTEST_ASSERT_GE(overallSimilarity,overallSimilarityScore);
        }
        else
        {
            for (int i=0; i<groundTruth.size(); i++)
            {
                GTEST_ASSERT_EQ(groundTruth[i],pixels[i]);
            }
        }
    }

public:
    ShaderPipelineTest()
    {
        object1Texture = utilities::loadTextureFromFile("resources/textures/gradient.jpg");
        object2Texture = utilities::loadTextureFromFile("resources/textures/transparent-test.png");
        defaultSampler = std::unique_ptr<Sampler>(Sampler::newSampler(SamplerParameters{}));


        std::vector<float> tverts =
        {
            -.5f,-.5f,0,
            0,.5f,0,
            .5f,-.5f,0,
        };
        std::vector<float> tuvs =
        {
            0,1,
            .5,0,
            1,1
        };

        std::vector<uint16_t> tindexes =
        {
            0,1,2
        };

        triangleVerts = std::unique_ptr<Buffer>(Buffer::newBuffer(tverts.data(),tverts.size()*sizeof(float),Buffer::Accessibility::GPU,Buffer::UsageFlags::VERTEX_BUFFER));
        triangleUVs = std::unique_ptr<Buffer>(Buffer::newBuffer(tuvs.data(),tuvs.size()*sizeof(float),Buffer::Accessibility::GPU,Buffer::UsageFlags::VERTEX_BUFFER));
        triangleIndicies = std::unique_ptr<Buffer>(Buffer::newBuffer(tindexes.data(),tindexes.size()*sizeof(uint16_t),Buffer::Accessibility::GPU,Buffer::UsageFlags::INDEX_BUFFER));

        vertexPosUVDescription.add(GraphicsType::VECTOR3,0,0).add(GraphicsType::VECTOR2,0,1);
    }
};

TEST_F(ShaderPipelineTest, DescriptorGroupReflection)
{
    ShaderFile stages[] =
    {
    {
            .pathIndicator = "resources/shaders/UnlitTextured.vertex",
            .stage = ShaderStageFlags::VERTEX,
        },
    {
            .pathIndicator = "resources/shaders/UnlitTextured.fragment",
            .stage = ShaderStageFlags::FRAGMENT,
        }
    };
    ShaderProperties properties{};
    VertexDescription vertexDescription(2);
    vertexDescription.add(GraphicsType::VECTOR3,0,0);
    vertexDescription.add(GraphicsType::VECTOR2,0,1);
    FrameBufferDescription frameBufferDescription;
    frameBufferDescription.colorTargets[0] = Pixels::Format::R8G8B8A8_UNORM;
    frameBufferDescription.depthTarget = Pixels::Format::D32_FLOAT;


    auto pipeline = GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(stages,2,properties,vertexDescription,frameBufferDescription);
    GTEST_ASSERT_EQ(pipeline->descriptorGroupCount(),3);
    auto group0 = pipeline->descriptorGroup(0);
    auto group1 = pipeline->descriptorGroup(1);
    auto group2 = pipeline->descriptorGroup(2);
    GTEST_ASSERT_EQ(group0->descriptorCount(),1);
    GTEST_ASSERT_EQ(group1->descriptorCount(),2);
    GTEST_ASSERT_EQ(group2->descriptorCount(),1);

    GTEST_ASSERT_TRUE(group0->descriptor(0).shape().type == Descriptor::Type::UNIFORM_BUFFER);
    GTEST_ASSERT_EQ(group0->descriptor(0).shape().arrayDepth,1);
    GTEST_ASSERT_EQ(group0->descriptor(0).shape().binding,0);

    GTEST_ASSERT_TRUE(group1->descriptor(0).shape().type == Descriptor::Type::UNIFORM_BUFFER);
    GTEST_ASSERT_EQ(group1->descriptor(0).shape().arrayDepth,1);
    GTEST_ASSERT_EQ(group1->descriptor(0).shape().binding,0);

    GTEST_ASSERT_TRUE(group1->descriptor(1).shape().type == Descriptor::Type::SAMPLER_AND_TEXTURE);
    GTEST_ASSERT_EQ(group1->descriptor(1).shape().arrayDepth,1);
    GTEST_ASSERT_EQ(group1->descriptor(1).shape().binding,1);

    GTEST_ASSERT_TRUE(group2->descriptor(0).shape().type == Descriptor::Type::UNIFORM_BUFFER);
    GTEST_ASSERT_EQ(group2->descriptor(0).shape().arrayDepth,1);
    GTEST_ASSERT_EQ(group2->descriptor(0).shape().binding,0);

    auto layout0_0 = pipeline->uniformBufferLayout(0,0);
    auto layout1_0 = pipeline->uniformBufferLayout(1,0);
    auto layout1_1 = pipeline->uniformBufferLayout(1,1);
    auto layout2_0 = pipeline->uniformBufferLayout(2,0);

    GTEST_ASSERT_EQ(layout0_0->childrenCount(),3);
    GTEST_ASSERT_EQ(layout0_0->size(),64*3);
    for (auto i=0; i<layout0_0->childrenCount(); i++)
    {
        auto& child = layout0_0->child(i);
        GTEST_ASSERT_TRUE(is4x4MatrixType(&child));
        GTEST_ASSERT_EQ(child.absoluteOffset(),64*i);
    }

    GTEST_ASSERT_EQ(layout1_0->childrenCount(),1);
    GTEST_ASSERT_EQ(layout1_0->size(),16);
    GTEST_ASSERT_EQ(layout1_0->child(0).type(), GraphicsType::VECTOR4);

    GTEST_ASSERT_EQ(layout1_1,nullptr);

    GTEST_ASSERT_EQ(layout2_0->size(),64);
    GTEST_ASSERT_TRUE(is4x4MatrixType(&layout2_0->child(0)));
}

TEST_F(ShaderPipelineTest, GraphicsPipelineThreadGroups)
{
    ShaderFile stages[] =
    {
        {
            .pathIndicator = "resources/shaders/UnlitTextured.vertex",
            .stage = ShaderStageFlags::VERTEX,
        },
    {
        .pathIndicator = "resources/shaders/UnlitTextured.fragment",
        .stage = ShaderStageFlags::FRAGMENT,
    }
    };
    ShaderProperties properties{};
    VertexDescription vertexDescription(2);
    vertexDescription.add(GraphicsType::VECTOR3,0,0);
    vertexDescription.add(GraphicsType::VECTOR2,0,1);
    FrameBufferDescription frameBufferDescription;
    frameBufferDescription.colorTargets[0] = Pixels::Format::R8G8B8A8_UNORM;
    frameBufferDescription.depthTarget = Pixels::Format::D32_FLOAT;


    auto pipeline = GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(stages,2,properties,vertexDescription,frameBufferDescription);
    GTEST_ASSERT_EQ(pipeline->xComputeThreads(),0);
    GTEST_ASSERT_EQ(pipeline->yComputeThreads(),0);
    GTEST_ASSERT_EQ(pipeline->zComputeThreads(),0);
}

TEST_F(ShaderPipelineTest, DescriptorGroupReflectionCompute)
{
    ShaderFile parallelAddFile{.pathIndicator = "resources/shaders/ParallelAdd", .stage = ShaderStageFlags::COMPUTE};
    auto computeAddParallel = GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(parallelAddFile);
    GTEST_ASSERT_EQ(computeAddParallel->descriptorGroupCount(),1);
    auto group0 = computeAddParallel->descriptorGroup(0);
    GTEST_ASSERT_EQ(group0->descriptorCount(),3);
    for (auto i=0; i<group0->descriptorCount(); i++)
    {
        auto& desc = group0->descriptor(i);
        GTEST_ASSERT_TRUE(desc.shape().type == Descriptor::Type::STORAGE_BUFFER);
    }

    ShaderFile computeDrawFile{.pathIndicator = "resources/shaders/ComputeDraw", .stage = ShaderStageFlags::COMPUTE};
    auto computeDraw = GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(computeDrawFile);
    GTEST_ASSERT_EQ(computeDraw->descriptorGroupCount(),1);
    group0 = computeDraw->descriptorGroup(0);
    GTEST_ASSERT_EQ(group0->descriptorCount(),1);
    auto& desc = group0->descriptor(0);
    GTEST_ASSERT_TRUE(desc.shape().type == Descriptor::Type::STORAGE_TEXTURE);
}

TEST_F(ShaderPipelineTest, ComputePipelineThreadGroups)
{
    ShaderFile parallelAddFile{.pathIndicator = "resources/shaders/ParallelAdd", .stage = ShaderStageFlags::COMPUTE};
    auto parallelAdd = GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(parallelAddFile);
    GTEST_ASSERT_EQ(parallelAdd->xComputeThreads(),1);
    GTEST_ASSERT_EQ(parallelAdd->yComputeThreads(),1);
    GTEST_ASSERT_EQ(parallelAdd->zComputeThreads(),1);
    ShaderFile computeDrawFile{.pathIndicator = "resources/shaders/ComputeDraw", .stage = ShaderStageFlags::COMPUTE};
    auto computeDraw = GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(computeDrawFile);
    GTEST_ASSERT_EQ(computeDraw->xComputeThreads(),16);
    GTEST_ASSERT_EQ(computeDraw->yComputeThreads(),16);
    GTEST_ASSERT_EQ(computeDraw->zComputeThreads(),1);
}

TEST_F(ShaderPipelineTest, MultiStageFlagFail)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    ShaderFile stages[] =
    {
        {
            .pathIndicator = "resources/shaders/UnlitTextured.vertex",
            .stage = ShaderStageFlags::VERTEX | ShaderStageFlags::MESH,
        },
    {
        .pathIndicator = "resources/shaders/UnlitTextured.fragment",
        .stage = ShaderStageFlags::FRAGMENT,
    }
    };

    ShaderProperties properties{};
    VertexDescription vertexDescription(2);
    vertexDescription.add(GraphicsType::VECTOR3,0,0);
    vertexDescription.add(GraphicsType::VECTOR2,0,1);
    FrameBufferDescription frameBufferDescription;
    frameBufferDescription.colorTargets[0] = Pixels::Format::R8G8B8A8_UNORM;
    frameBufferDescription.depthTarget = Pixels::Format::D32_FLOAT;


    EXPECT_DEATH(GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(stages,2,properties,vertexDescription,frameBufferDescription),"Only one stage can be set per Shader Code instance");
}

TEST_F(ShaderPipelineTest, DepthClamp)
{
    ShaderProperties properties{};
    properties.rasterizationState.depthClampEnable = true;

    ShaderProperties properties2{};
    properties2.rasterizationState.depthClampEnable = false;


    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.4f));
    object1 = glm::rotate(object1,glm::radians(-45.0f),glm::vec3(0,1,0));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f,-.4f));
    object2 = glm::rotate(object2,glm::radians(45.0f),glm::vec3(0,1,0));


    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::perspective(glm::radians(105.0f), 1.0f, 0.1f, 100.0f);

    testProperties(properties,properties2,cameraTransform,cameraProjection,object1,object2,"resources/textures/depth-clamp-result.png",.98,0);

}

TEST_F(ShaderPipelineTest,RasterizationDiscard)
{
    GTEST_SKIP("I'm not smart enough to test this, I've read what it does, and... I don't know how to capture the output? I guess it could be done with a generic write buffer as a descriptor group item?");
    return;
    ShaderProperties properties{};
    properties.rasterizationState.rasterizerDicardEnable = true;

    ShaderProperties properties2{};

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.5f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.5f, 0.0f,-.4f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties2,cameraTransform,cameraProjection,object1,object2,"resources/textures/rasterizer-discard-result.png",.98,0);
}

TEST_F(ShaderPipelineTest,DrawFace)
{
    ShaderProperties properties{};
    properties.rasterizationState.drawMode = RasterizationState::DrawMode::FACE;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f,-.6f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/draw-face-result.png",.98,0);
}

TEST_F(ShaderPipelineTest,DrawEdges)
{
    ShaderProperties properties{};
    properties.rasterizationState.drawMode = RasterizationState::DrawMode::EDGE;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f,-.6f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/draw-edge-result.png",.999,.3);
}
TEST_F(ShaderPipelineTest,DrawVerticies)
{
    ShaderProperties properties{};
    properties.rasterizationState.drawMode = RasterizationState::DrawMode::VERTEX;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f,-.6f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/draw-verticies-result.png",1,1);
}
TEST_F(ShaderPipelineTest,DrawThicknessEdges)
{
    ShaderProperties properties{};
    properties.rasterizationState.drawMode = RasterizationState::DrawMode::EDGE;
    properties.rasterizationState.lineThickness = 5.0f;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f,-.6f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/draw-verticies-result.png",.97, .3);
}

TEST_F(ShaderPipelineTest,CullNone)
{
    ShaderProperties properties{};
    properties.rasterizationState.culling = RasterizationState::CullOptions::NONE;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.6f));
    object2 = glm::rotate(object2, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/cull-none-result.png",.98,0);
}
TEST_F(ShaderPipelineTest,CullFront)
{
    ShaderProperties properties{};
    properties.rasterizationState.culling = RasterizationState::CullOptions::FRONT_FACING;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.6f));
    object2 = glm::rotate(object2, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/cull-front-result.png",.99,0);
}
TEST_F(ShaderPipelineTest,CullBack)
{
    ShaderProperties properties{};
    properties.rasterizationState.culling = RasterizationState::CullOptions::BACK_FACING;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.6f));
    object2 = glm::rotate(object2, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/cull-back-result.png",.98,0);
}

TEST_F(ShaderPipelineTest,FrontFaceClockWise)
{
    ShaderProperties properties{};
    properties.rasterizationState.culling = RasterizationState::CullOptions::BACK_FACING;
    properties.rasterizationState.frontFacing = RasterizationState::FrontFacing::CLOCKWISE;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.6f));
    object2 = glm::rotate(object2, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/front-face-clockwise-result.png",.98,0);
}

TEST_F(ShaderPipelineTest,FrontFaceCounterClockWise)
{
    ShaderProperties properties{};
    properties.rasterizationState.culling = RasterizationState::CullOptions::BACK_FACING;
    properties.rasterizationState.frontFacing = RasterizationState::FrontFacing::COUNTER_CLOCKWISE;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.6f));
    object2 = glm::rotate(object2, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/front-face-counter-clockwise-result.png",.99,0);
}

TEST_F(ShaderPipelineTest,DepthBias)
{
    ShaderProperties properties{};
    ShaderProperties properties2{};
    properties2.rasterizationState.depthBiasEnable = true;
    properties2.rasterizationState.depthBiasConstantFactor = 1;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.5f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties2,cameraTransform,cameraProjection,object1,object2,"resources/textures/depth-bias-result.png",.99,0);
}

TEST_F(ShaderPipelineTest, DepthBiasWithSlope)
{
    ShaderProperties properties{};
    ShaderProperties properties2{};
    properties2.rasterizationState.depthBiasEnable = true;
    properties2.rasterizationState.depthBiasConstantFactor = -1;
    properties2.rasterizationState.depthBiasSlopeFactor = -1;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));
    object1 = glm::rotate(object1,glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 object2 = glm::rotate(object1,glm::radians(.05f), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);
    testProperties(properties,properties2,cameraTransform,cameraProjection,object1,object2,"resources/textures/depth-bias-result.png",.9,0);
}

TEST_F(ShaderPipelineTest,MultiSample)
{
    ShaderProperties properties{};
    properties.multiSampleState.alphaToOneEnable = false;
    properties.multiSampleState.sampleShadingEnable = true;
    properties.multiSampleState.rasterizationSamples = 8;
    properties.multiSampleState.minSampleShading = 4;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.4f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testPropertiesMultiSample(properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/multi-sample-result.png",.99,.5);
}

TEST_F(ShaderPipelineTest,MultiSampleAlpha)
{
    ShaderProperties properties{};
    properties.multiSampleState.alphaToOneEnable = true;
    properties.multiSampleState.sampleShadingEnable = true;
    properties.multiSampleState.rasterizationSamples = 8;
    properties.multiSampleState.minSampleShading = 4;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.4f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testPropertiesMultiSample(properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/multi-sample-alpha-result.png",.99,.5);
}

TEST_F(ShaderPipelineTest,BlendStateDisable)
{
    ShaderProperties properties{};
    properties.blendState.attachmentBlendStates[0].blendingEnabled = false;
    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.4f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/blend-disabled-result.png",.99,0);
}

TEST_F(ShaderPipelineTest,BlendStateDefaults)
{
    ShaderProperties properties{};

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.4f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/draw-face-result.png",.99,0);

}

TEST_F(ShaderPipelineTest,BlendStateColorMinus)
{
    ShaderProperties properties{};
    properties.blendState.attachmentBlendStates[0].colorBlendOperation = Operations::BlendOperation::BLEND_OP_SUBTRACT;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.4f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/color-blend-minus-result.png",.99,0);

}

TEST_F(ShaderPipelineTest,BlendStateAlphaMinus)
{
    ShaderProperties properties{};
    properties.blendState.attachmentBlendStates[0].alphaBlendOperation = Operations::BlendOperation::BLEND_OP_SUBTRACT;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.4f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/alpha-blend-minus.png",.99,0);

}

TEST_F(ShaderPipelineTest,BlendStateColorMax)
{
    ShaderProperties properties{};
    properties.blendState.attachmentBlendStates[0].colorBlendOperation = Operations::BlendOperation::BLEND_OP_MAX;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.4f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/color-blend-max-result.png",.99,0);

}

TEST_F(ShaderPipelineTest,BlendStateAlphaMax)
{
    ShaderProperties properties{};
    properties.blendState.attachmentBlendStates[0].alphaBlendOperation = Operations::BlendOperation::BLEND_OP_MAX;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.4f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/alpha-blend-max-result.png",.99,0);

}

TEST_F(ShaderPipelineTest,BlendStateColorMin)
{
    ShaderProperties properties{};
    properties.blendState.attachmentBlendStates[0].colorBlendOperation = Operations::BlendOperation::BLEND_OP_MIN;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.4f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/color-blend-min-result.png",.99,0);

}

TEST_F(ShaderPipelineTest,BlendStateAlphaMin)
{
    ShaderProperties properties{};
    properties.blendState.attachmentBlendStates[0].alphaBlendOperation = Operations::BlendOperation::BLEND_OP_MIN;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.4f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/alpha-blend-min-result.png",.99,0);

}

TEST_F(ShaderPipelineTest,BlendStateColorReverseMinus)
{
    ShaderProperties properties{};
    properties.blendState.attachmentBlendStates[0].colorBlendOperation = Operations::BlendOperation::BLEND_OP_REVERSE_SUBTRACT;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.4f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/color-blend-reverse-minus-result.png",.99,0);

}

TEST_F(ShaderPipelineTest,BlendStateAlphaReverseMinus)
{
    ShaderProperties properties{};
    properties.blendState.attachmentBlendStates[0].alphaBlendOperation = Operations::BlendOperation::BLEND_OP_REVERSE_SUBTRACT;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.4f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/alpha-blend-reverse-minus-result.png",.99,0);
}

TEST_F(ShaderPipelineTest,BlendStateColorWriteMask)
{
    ShaderProperties properties{};
    properties.blendState.attachmentBlendStates[0].colorWriteMask = Color::ComponentFlags::RED_COMPONENT | Color::ComponentFlags::GREEN_COMPONENT;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.4f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/blend-color-write-mask-result.png",.99,0);
}

TEST_F(ShaderPipelineTest,BlendStateBlendFactors)
{
    //TODO: There's too many combinations to test, the combinatrics easily explode to unimaginable numbers, I think maybe I should just test a few? There are 96 tests to do even if I do them one at a time, and I don't think even that is practical. (even small images take up some space)
    GTEST_SKIP("Not feasible to test");
}

TEST_F(ShaderPipelineTest,BlendStateLogicOpClear)
{
    ShaderProperties properties{};
    properties.blendState.logicOperationEnable = true;
    properties.blendState.logicalOperation = Operations::LogicalOperation::LOGIC_OP_CLEAR;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.4f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-clear-result.png",.90,0);
}

TEST_F(ShaderPipelineTest,BlendStateLogicOpInverted)
{
    ShaderProperties properties{};
    properties.blendState.logicOperationEnable = true;
    properties.blendState.logicalOperation = Operations::LogicalOperation::LOGIC_OP_COPY_INVERTED;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.4f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-inverted-result.png",.90,0);
}

TEST_F(ShaderPipelineTest,BlendStateLogicOpEquivalent)
{
    ShaderProperties properties{};
    properties.blendState.logicOperationEnable = true;
    properties.blendState.logicalOperation = Operations::LogicalOperation::LOGIC_OP_EQUIVALENT;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.4f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-equivalent-result.png",.90,0);
}

TEST_F(ShaderPipelineTest,BlendStateLogicOpOr)
{
    ShaderProperties properties{};
    properties.blendState.logicOperationEnable = true;
    properties.blendState.logicalOperation = Operations::LogicalOperation::LOGIC_OP_OR;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.4f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-or-result.png",.90,0);
}

TEST_F(ShaderPipelineTest,BlendStateLogicOpAnd)
{
    ShaderProperties properties{};
    properties.blendState.logicOperationEnable = true;
    properties.blendState.logicalOperation = Operations::LogicalOperation::LOGIC_OP_AND;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.4f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-and-result.png",.90,0);
}

TEST_F(ShaderPipelineTest,BlendStateLogicOpAndInverted)
{
    ShaderProperties properties{};
    properties.blendState.logicOperationEnable = true;
    properties.blendState.logicalOperation = Operations::LogicalOperation::LOGIC_OP_AND_INVERTED;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.4f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-and-inverted-result.png",.90,0);
}

TEST_F(ShaderPipelineTest,BlendStateLogicOpAndReverse)
{
    ShaderProperties properties{};
    properties.blendState.logicOperationEnable = true;
    properties.blendState.logicalOperation = Operations::LogicalOperation::LOGIC_OP_AND_REVERSE;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.4f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-and-reverse-result.png",.90,0);
}

TEST_F(ShaderPipelineTest,BlendStateLogicOpCopy)
{
    ShaderProperties properties{};
    properties.blendState.logicOperationEnable = true;
    properties.blendState.logicalOperation = Operations::LogicalOperation::LOGIC_OP_COPY;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.4f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-copy-result.png",.90,0);
}

TEST_F(ShaderPipelineTest,BlendStateLogicOpInvert)
{
    ShaderProperties properties{};
    properties.blendState.logicOperationEnable = true;
    properties.blendState.logicalOperation = Operations::LogicalOperation::LOGIC_OP_INVERT;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.4f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-invert-result.png",.90,0);
}

TEST_F(ShaderPipelineTest,BlendStateLogicOpNAnd)
{
    ShaderProperties properties{};
    properties.blendState.logicOperationEnable = true;
    properties.blendState.logicalOperation = Operations::LogicalOperation::LOGIC_OP_NAND;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.4f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-nand-result.png",.90,0);
}

TEST_F(ShaderPipelineTest,BlendStateLogicOpNoOp)
{
    ShaderProperties properties{};
    properties.blendState.logicOperationEnable = true;
    properties.blendState.logicalOperation = Operations::LogicalOperation::LOGIC_OP_NO_OP;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.4f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-no-op-result.png",.90,0);
}

TEST_F(ShaderPipelineTest,BlendStateLogicOpNor)
{
    ShaderProperties properties{};
    properties.blendState.logicOperationEnable = true;
    properties.blendState.logicalOperation = Operations::LogicalOperation::LOGIC_OP_NOR;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.4f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-nor-result.png",.90,0);
}

TEST_F(ShaderPipelineTest,BlendStateLogicOpOrInverted)
{
    ShaderProperties properties{};
    properties.blendState.logicOperationEnable = true;
    properties.blendState.logicalOperation = Operations::LogicalOperation::LOGIC_OP_OR_INVERTED;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.4f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-or-inverted-result.png",.90,0);
}

TEST_F(ShaderPipelineTest,BlendStateLogicOpReverse)
{
    ShaderProperties properties{};
    properties.blendState.logicOperationEnable = true;
    properties.blendState.logicalOperation = Operations::LogicalOperation::LOGIC_OP_REVERSE;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.4f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-reverse-result.png",.90,0);
}

TEST_F(ShaderPipelineTest,BlendStateLogicOpSet)
{
    ShaderProperties properties{};
    properties.blendState.logicOperationEnable = true;
    properties.blendState.logicalOperation = Operations::LogicalOperation::LOGIC_OP_SET;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.4f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-set-result.png",.90,0);
}

TEST_F(ShaderPipelineTest,BlendStateLogicOpXOr)
{
    ShaderProperties properties{};
    properties.blendState.logicOperationEnable = true;
    properties.blendState.logicalOperation = Operations::LogicalOperation::LOGIC_OP_XOR;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.4f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-xor-result.png",.90,0);
}

TEST_F(ShaderPipelineTest,DepthStencilStateDepthDisable)
{
    ShaderProperties properties{};
    properties.depthStencilState.depthTestEnable = false;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.6f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/draw-face-result.png",.99,.8);
}

TEST_F(ShaderPipelineTest,DepthStencilStateComparisonFunctionAlways)
{
    ShaderProperties properties{};
    properties.depthStencilState.depthCompareOperation = Operations::ComparisonFunction::COMPARISION_ALWAYS;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.6f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/draw-face-result.png",.99,.8);
}
TEST_F(ShaderPipelineTest,DepthStencilStateComparisonGreater)
{
    ShaderProperties properties{};
    properties.depthStencilState.depthCompareOperation = Operations::ComparisonFunction::COMPARISION_GREATER;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.6f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-no-op-result.png",.99,.8);
}

TEST_F(ShaderPipelineTest,DepthStencilStateComparisonEqual)
{
    ShaderProperties properties{};
    properties.depthStencilState.depthCompareOperation = Operations::ComparisonFunction::COMPARISION_EQUAL;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -1.0f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, 0.0f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/cull-back-result.png",.99,.8);
}

TEST_F(ShaderPipelineTest,DepthStencilStateComparisonNever)
{
    ShaderProperties properties{};
    properties.depthStencilState.depthCompareOperation = Operations::ComparisonFunction::COMPARISION_NEVER;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.6f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-no-op-result.png",.99,.8);
}

TEST_F(ShaderPipelineTest,DepthStencilStateComparisonNotEqual)
{
    ShaderProperties properties{};
    properties.depthStencilState.depthCompareOperation = Operations::ComparisonFunction::COMPARISION_NOT_EQUAL;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.6f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/draw-face-result.png",.99,.8);
}

TEST_F(ShaderPipelineTest,DepthStencilStateDepthWriteEnableFalse)
{
    ShaderProperties properties{};
    properties.depthStencilState.depthWriteEnable = false;

    glm::mat4 object1 = glm::mat4(1.0f);
    object1 = glm::translate(object1, glm::vec3(-.25f, 0.0f, -.5f));

    glm::mat4 object2(1.0f);
    object2 = glm::translate(object2, glm::vec3(.25f, 0.0f, -.6f));

    glm::mat4 cameraTransform(1.0f);
    glm::mat4 cameraProjection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/draw-face-result.png",.99,.8);
}

TEST_F(ShaderPipelineTest,DepthStencilStateStencilDetails)
{
    //TODO: There's too many combinations to test
    GTEST_SKIP("Not feasible to test");
}