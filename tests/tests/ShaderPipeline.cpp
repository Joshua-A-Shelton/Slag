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
bool is4x4MatrixType(const BufferLayout* layout)
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
        framebufferDescription.depthTarget = Pixels::Format::D32_FLOAT_S8X24_UINT;
        auto shader1 = GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(files,2,properties1,vertexPosUVDescription,framebufferDescription);
        auto shader2 = GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(files,2,properties2,vertexPosUVDescription,framebufferDescription);
        auto resourceMemory = std::unique_ptr<ResourceDescriptorMemory>(ResourceDescriptorMemory::newResourceDescriptorMemory(100));
        auto samplerMemory = std::unique_ptr<SamplerDescriptorMemory>(SamplerDescriptorMemory::newSamplerDescriptorMemory(100));
        auto target = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::RENDER_TARGET_ATTACHMENT,imageSize,imageSize,1,1,1));
        auto depth = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::D32_FLOAT_S8X24_UINT,Texture::Type::TEXTURE_2D,Texture::UsageFlags::DEPTH_STENCIL_ATTACHMENT,imageSize,imageSize,1,1,1));
        auto targetOutput = std::unique_ptr<Buffer>(Buffer::newBuffer(target->byteSize(Pixels::AspectFlags::COLOR),Buffer::Accessibility::CPU_AND_GPU));

        auto finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));

        uint64_t globalsDescriptorGroup = 0;
        uint64_t globalsIndex = shader1->descriptorGroup(0)->descriptorByteOffset(0);
        uint64_t samplerDescriptorGroup = 0;
        uint64_t samplerIndex = shader1->descriptorGroup(1)->descriptorByteOffset(0);
        uint64_t object1DescriptorGroup = resourceMemory->nextDescriptorGroupOffset(shader1->descriptorGroup(0)->descriptorBufferSize());
        uint64_t object1BufferIndex = object1DescriptorGroup + shader1->descriptorGroup(2)->descriptorByteOffset(0);
        uint64_t object1TextureIndex = object1DescriptorGroup + shader1->descriptorGroup(2)->descriptorByteOffset(1);
        uint64_t object2DescriptorGroup = resourceMemory->nextDescriptorGroupOffset(object1DescriptorGroup + shader1->descriptorGroup(2)->descriptorBufferSize());
        uint64_t object2BufferIndex = object2DescriptorGroup + shader2->descriptorGroup(2)->descriptorByteOffset(0);
        uint64_t object2TextureIndex = object2DescriptorGroup + shader2->descriptorGroup(2)->descriptorByteOffset(1);

        resourceMemory->setUniformBuffer(globalsIndex,globalsBuffer.get(),0,globalsBuffer->size());

        samplerMemory->setSampler(samplerIndex,defaultSampler.get());

        resourceMemory->setUniformBuffer(object1BufferIndex,objectBuffer.get(),0,sizeof(TexturedDepthSet1Group));
        resourceMemory->setSampledTexture(object1TextureIndex,object1Texture.get());

        resourceMemory->setUniformBuffer(object2BufferIndex,objectBuffer.get(),sizeof(TexturedDepthSet1Group),sizeof(TexturedDepthSet1Group));
        resourceMemory->setSampledTexture(object2TextureIndex,object2Texture.get());


        commandBuffer->begin();

        commandBuffer->bindDescriptorMemory(resourceMemory.get(),samplerMemory.get());
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
        commandBuffer->bindGraphicsDescriptorGroup(0,resourceMemory.get(),globalsDescriptorGroup);
        commandBuffer->bindGraphicsDescriptorGroup(1,samplerMemory.get(),samplerDescriptorGroup);
        commandBuffer->bindGraphicsDescriptorGroup(2,resourceMemory.get(),object1DescriptorGroup);
        commandBuffer->drawIndexed(triangleVerts->countAsArray<glm::vec3>(),1,0,0,0);
        commandBuffer->bindGraphicsShaderPipeline(shader2.get());
        commandBuffer->bindGraphicsDescriptorGroup(2,resourceMemory.get(),object2DescriptorGroup);
        commandBuffer->drawIndexed(triangleVerts->countAsArray<glm::vec3>(),1,0,0,0);

        commandBuffer->endRendering();

        commandBuffer->insertBarrier(TextureBarrier{.texture = target.get(),.baseLayer = 0,.layerCount = 1,.baseMipLevel = 0,.mipCount = 1,.accessBefore = BarrierAccessFlags::SHADER_WRITE,.accessAfter = BarrierAccessFlags::TRANSFER_READ,.syncBefore = PipelineStageFlags::ALL_GRAPHICS,.syncAfter = PipelineStageFlags::TRANSFER});

        TextureBufferMapping copyData{.bufferOffset = 0, .textureSubresource = TextureSubresource{Pixels::AspectFlags::COLOR,0,0,1},.textureOffset = {0,0,0},.textureExtent = {target->width(),target->height(),1}};
        commandBuffer->copyTextureToBuffer(target.get(),targetOutput.get(),&copyData,1);

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

        GTEST_ASSERT_TRUE(utilities::matchesSimilarity(targetOutput.get(),compareResult,overallSimilarityScore,individualPixelScore));

    }
    void testPropertiesMultiSample(ShaderProperties properties,glm::mat4 cameraTransform, glm::mat4 cameraProjection, glm::mat4 object1Transform, glm::mat4 object2Transform, const std::filesystem::path& compareResult, float overallSimilarityScore, float individualPixelScore)
    {
        auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::GRAPHICS));
        auto finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore());
        auto target = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::RENDER_TARGET_ATTACHMENT,150,150,1,1,1,Texture::SampleCount::EIGHT));
        auto depth = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::D32_FLOAT,Texture::Type::TEXTURE_2D,Texture::UsageFlags::DEPTH_STENCIL_ATTACHMENT,150,150,1,1,1,Texture::SampleCount::EIGHT));
        auto final = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::RENDER_TARGET_ATTACHMENT,150,150, 1,1,1));

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



        auto resourceMemory = std::unique_ptr<ResourceDescriptorMemory>(ResourceDescriptorMemory::newResourceDescriptorMemory(100));
        auto samplerMemory = std::unique_ptr<SamplerDescriptorMemory>(SamplerDescriptorMemory::newSamplerDescriptorMemory(100));
        auto targetOutput = std::unique_ptr<Buffer>(Buffer::newBuffer(final->byteSize(Pixels::AspectFlags::COLOR),Buffer::Accessibility::CPU_AND_GPU));


        uint64_t globalsDescriptorGroup = 0;
        uint64_t globalsIndex = shader->descriptorGroup(0)->descriptorByteOffset(0);
        uint64_t samplerDescriptorGroup = 0;
        uint64_t samplerIndex = shader->descriptorGroup(1)->descriptorByteOffset(0);
        uint64_t object1DescriptorGroup = resourceMemory->nextDescriptorGroupOffset(shader->descriptorGroup(0)->descriptorBufferSize());
        uint64_t object1BufferIndex = object1DescriptorGroup + shader->descriptorGroup(2)->descriptorByteOffset(0);
        uint64_t object1TextureIndex = object1DescriptorGroup + shader->descriptorGroup(2)->descriptorByteOffset(1);
        uint64_t object2DescriptorGroup = resourceMemory->nextDescriptorGroupOffset(object1DescriptorGroup + shader->descriptorGroup(2)->descriptorBufferSize());
        uint64_t object2BufferIndex = object2DescriptorGroup + shader->descriptorGroup(2)->descriptorByteOffset(0);
        uint64_t object2TextureIndex = object2DescriptorGroup + shader->descriptorGroup(2)->descriptorByteOffset(1);


        resourceMemory->setUniformBuffer(globalsIndex,globalsBuffer.get(),0,globalsBuffer->size());

        samplerMemory->setSampler(samplerIndex,defaultSampler.get());

        resourceMemory->setUniformBuffer(object1BufferIndex,objectBuffer.get(),0,sizeof(TexturedDepthSet1Group));
        resourceMemory->setSampledTexture(object1TextureIndex,object1Texture.get());

        resourceMemory->setUniformBuffer(object2BufferIndex,objectBuffer.get(),sizeof(TexturedDepthSet1Group),sizeof(TexturedDepthSet1Group));
        resourceMemory->setSampledTexture(object2TextureIndex,object2Texture.get());



        commandBuffer->begin();

        commandBuffer->bindDescriptorMemory(resourceMemory.get(),samplerMemory.get());
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

        commandBuffer->bindGraphicsDescriptorGroup(0,resourceMemory.get(),globalsDescriptorGroup);
        commandBuffer->bindGraphicsDescriptorGroup(1,samplerMemory.get(),samplerDescriptorGroup);
        commandBuffer->bindGraphicsDescriptorGroup(2,resourceMemory.get(),object1DescriptorGroup);
        commandBuffer->drawIndexed(triangleVerts->countAsArray<glm::vec3>(),1,0,0,0);
        commandBuffer->bindGraphicsDescriptorGroup(2,resourceMemory.get(),object2DescriptorGroup);
        commandBuffer->drawIndexed(triangleVerts->countAsArray<glm::vec3>(),1,0,0,0);

        commandBuffer->endRendering();

        commandBuffer->insertBarrier(TextureBarrier{.texture = target.get(),.baseLayer = 0,.layerCount = 1,.baseMipLevel = 0,.mipCount = 1,.accessBefore = BarrierAccessFlags::SHADER_WRITE,.accessAfter = BarrierAccessFlags::BLIT_READ,.syncBefore = PipelineStageFlags::ALL_GRAPHICS,.syncAfter = PipelineStageFlags::BLIT});

        commandBuffer->resolve(target.get(),0,0,{0,0},final.get(),0,0,{0,0},{150,150});

        commandBuffer->insertBarrier(TextureBarrier{.texture = final.get(),.baseLayer = 0,.layerCount = 1,.baseMipLevel = 0,.mipCount = 1,.accessBefore = BarrierAccessFlags::BLIT_WRITE,.accessAfter = BarrierAccessFlags::TRANSFER_READ,.syncBefore = PipelineStageFlags::BLIT,.syncAfter = PipelineStageFlags::TRANSFER});



        TextureBufferMapping copyData{.bufferOffset = 0, .textureSubresource = TextureSubresource{Pixels::AspectFlags::COLOR,0,0,1}, .textureOffset = {0,0,0}, .textureExtent = {final->width(),final->height(),1}};
        commandBuffer->copyTextureToBuffer(final.get(),targetOutput.get(),&copyData,1);

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
    GTEST_ASSERT_EQ(pipeline->descriptorGroupCount(),4);
    auto group0 = pipeline->descriptorGroup(0);
    auto group1 = pipeline->descriptorGroup(1);
    auto group2 = pipeline->descriptorGroup(2);
    auto group3 = pipeline->descriptorGroup(3);
    GTEST_ASSERT_EQ(group0->descriptorCount(),1);
    GTEST_ASSERT_EQ(group1->descriptorCount(),1);
    GTEST_ASSERT_EQ(group2->descriptorCount(),2);
    GTEST_ASSERT_EQ(group3->descriptorCount(),1);

    GTEST_ASSERT_TRUE(group0->descriptor(0).shape().type == Descriptor::Type::UNIFORM_BUFFER);
    GTEST_ASSERT_EQ(group0->descriptor(0).shape().arrayDepth,1);

    GTEST_ASSERT_TRUE(group1->descriptor(0).shape().type == Descriptor::Type::SAMPLER);
    GTEST_ASSERT_EQ(group1->descriptor(0).shape().arrayDepth,1);

    GTEST_ASSERT_TRUE(group2->descriptor(0).shape().type == Descriptor::Type::UNIFORM_BUFFER);
    GTEST_ASSERT_EQ(group2->descriptor(0).shape().arrayDepth,1);

    GTEST_ASSERT_EQ(group2->descriptor(1).shape().type, Descriptor::Type::SAMPLED_TEXTURE);
    GTEST_ASSERT_EQ(group2->descriptor(1).shape().arrayDepth,1);

    GTEST_ASSERT_TRUE(group3->descriptor(0).shape().type == Descriptor::Type::UNIFORM_BUFFER);
    GTEST_ASSERT_EQ(group3->descriptor(0).shape().arrayDepth,1);

    auto layout0_0 = pipeline->uniformBufferLayout(0,0);
    auto layout2_0 = pipeline->uniformBufferLayout(2,0);
    auto layout2_1 = pipeline->uniformBufferLayout(2,1);
    auto layout3_0 = pipeline->uniformBufferLayout(3,0);

    GTEST_ASSERT_EQ(layout0_0->childrenCount(),3);
    GTEST_ASSERT_EQ(layout0_0->size(),64*3);
    for (auto i=0; i<layout0_0->childrenCount(); i++)
    {
        auto& child = layout0_0->child(i);
        GTEST_ASSERT_TRUE(is4x4MatrixType(&child));
        GTEST_ASSERT_EQ(child.absoluteOffset(),64*i);
    }

    GTEST_ASSERT_EQ(layout2_0->childrenCount(),1);
    GTEST_ASSERT_EQ(layout2_0->size(),16);
    GTEST_ASSERT_EQ(layout2_0->child(0).type(), GraphicsType::VECTOR4);

    GTEST_ASSERT_EQ(layout2_1,nullptr);

    GTEST_ASSERT_EQ(layout3_0->size(),64);
    GTEST_ASSERT_TRUE(is4x4MatrixType(&layout3_0->child(0)));


}
TEST_F(ShaderPipelineTest, DescriptorGroupReflectionAllTypes)
{
    ShaderFile stages[] =
        {
        {
            .pathIndicator = "resources/shaders/AllParameterTypes.vertex",
            .stage = ShaderStageFlags::VERTEX,
        },
     {
         .pathIndicator = "resources/shaders/AllParameterTypes.fragment",
         .stage = ShaderStageFlags::FRAGMENT,
     }
        };
    ShaderProperties properties{};
    VertexDescription vertexDescription(2);
    vertexDescription.add(GraphicsType::VECTOR3, 0, 0);
    vertexDescription.add(GraphicsType::VECTOR2, 0, 1);
    FrameBufferDescription frameBufferDescription;
    frameBufferDescription.colorTargets[0] = Pixels::Format::R8G8B8A8_UNORM;
    frameBufferDescription.depthTarget = Pixels::Format::D32_FLOAT;


    auto pipeline = GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(stages, 2, properties, vertexDescription, frameBufferDescription);
    GTEST_ASSERT_EQ(pipeline->descriptorGroupCount(),3);
    auto group0 = pipeline->descriptorGroup(0);
    auto group1 = pipeline->descriptorGroup(1);
    auto group2 = pipeline->descriptorGroup(2);
    GTEST_ASSERT_EQ(group0->descriptorCount(),3);
    GTEST_ASSERT_EQ(group1->descriptorCount(),3);
    GTEST_ASSERT_EQ(group2->descriptorCount(),1);

    GTEST_ASSERT_TRUE(group0->descriptor(0).shape().type == Descriptor::Type::UNIFORM_BUFFER);
    GTEST_ASSERT_EQ(group0->descriptor(0).shape().arrayDepth,1);
    GTEST_ASSERT_EQ(group0->descriptor(0).shape().dimension,Descriptor::Dimension::ONE_DIMENSIONAL);

    GTEST_ASSERT_TRUE(group0->descriptor(1).shape().type == Descriptor::Type::SAMPLED_TEXTURE);
    GTEST_ASSERT_EQ(group0->descriptor(1).shape().arrayDepth,1);
    GTEST_ASSERT_EQ(group0->descriptor(1).shape().dimension,Descriptor::Dimension::TWO_DIMENSIONAL);

    GTEST_ASSERT_TRUE(group0->descriptor(2).shape().type == Descriptor::Type::STORAGE_TEXTURE);
    GTEST_ASSERT_EQ(group0->descriptor(2).shape().arrayDepth,1);
    GTEST_ASSERT_EQ(group0->descriptor(2).shape().dimension,Descriptor::Dimension::TWO_DIMENSIONAL);

    GTEST_ASSERT_TRUE(group1->descriptor(0).shape().type == Descriptor::Type::UNIFORM_TEXEL_BUFFER);
    GTEST_ASSERT_EQ(group1->descriptor(0).shape().arrayDepth,1);
    GTEST_ASSERT_EQ(group1->descriptor(0).shape().dimension,Descriptor::Dimension::ONE_DIMENSIONAL);

    GTEST_ASSERT_TRUE(group1->descriptor(1).shape().type == Descriptor::Type::STORAGE_TEXEL_BUFFER);
    GTEST_ASSERT_EQ(group1->descriptor(1).shape().arrayDepth,2);
    GTEST_ASSERT_EQ(group1->descriptor(1).shape().dimension,Descriptor::Dimension::ONE_DIMENSIONAL);

    GTEST_ASSERT_TRUE(group1->descriptor(2).shape().type == Descriptor::Type::STORAGE_BUFFER);
    GTEST_ASSERT_EQ(group1->descriptor(2).shape().arrayDepth,1);
    GTEST_ASSERT_EQ(group1->descriptor(2).shape().dimension,Descriptor::Dimension::ONE_DIMENSIONAL);

    GTEST_ASSERT_TRUE(group2->descriptor(0).shape().type == Descriptor::Type::SAMPLER);
    GTEST_ASSERT_EQ(group2->descriptor(0).shape().arrayDepth,1);
    GTEST_ASSERT_EQ(group2->descriptor(0).shape().dimension,Descriptor::Dimension::ONE_DIMENSIONAL);

    auto uniformBufferLayout = pipeline->uniformBufferLayout(0,0);
    auto uniformTexelBufferLayout = pipeline->texelBufferDescription(1,0);
    auto storageTexelBufferLayout = pipeline->texelBufferDescription(1,1);
    auto storageBufferLayout = pipeline->uniformBufferLayout(1,2);

    GTEST_ASSERT_NE(uniformBufferLayout,nullptr);
    GTEST_ASSERT_NE(uniformTexelBufferLayout,nullptr);
    GTEST_ASSERT_NE(storageTexelBufferLayout,nullptr);
    GTEST_ASSERT_NE(storageBufferLayout,nullptr);

    GTEST_ASSERT_EQ(uniformBufferLayout->childrenCount(),1);
    GTEST_ASSERT_EQ(uniformBufferLayout->child(0).type(),GraphicsType::VECTOR4);

    GTEST_ASSERT_EQ(uniformTexelBufferLayout->format(),Pixels::Format::R32G32B32A32_FLOAT);

    GTEST_ASSERT_EQ(storageTexelBufferLayout->format(),Pixels::Format::R32G32B32A32_UINT);

    GTEST_ASSERT_EQ(storageBufferLayout->childrenCount(),1);
    GTEST_ASSERT_EQ(storageBufferLayout->child(0).type(),GraphicsType::STRUCT);
    GTEST_ASSERT_EQ(storageBufferLayout->child(0).childrenCount(),2);
    GTEST_ASSERT_EQ(storageBufferLayout->child(0)[0].type(),GraphicsType::FLOAT);
    GTEST_ASSERT_EQ(storageBufferLayout->child(0)[1].type(),GraphicsType::INTEGER);

}

TEST_F(ShaderPipelineTest, NoPushConstants)
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
    GTEST_ASSERT_EQ(pipeline->pushConstants(),nullptr);
}

TEST_F(ShaderPipelineTest, PushConstants)
{
    ShaderFile stages[] =
    {
        {
            .pathIndicator = "resources/shaders/PushConstants.vertex",
            .stage = ShaderStageFlags::VERTEX,
        },
     {
         .pathIndicator = "resources/shaders/PushConstants.fragment",
         .stage = ShaderStageFlags::FRAGMENT,
         }
    };
    ShaderProperties properties{};
    VertexDescription vertexDescription(1);
    vertexDescription.add(GraphicsType::VECTOR3,0,0);
    FrameBufferDescription frameBufferDescription;
    frameBufferDescription.colorTargets[0] = Pixels::Format::R8G8B8A8_UNORM;

    auto pipeline = GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(stages,2,properties,vertexDescription,frameBufferDescription);
    auto pushConstants = pipeline->pushConstants();
    GTEST_ASSERT_NE(pushConstants,nullptr);
    GTEST_ASSERT_EQ(pushConstants->childrenCount(),3);
    GTEST_ASSERT_EQ(pushConstants->offset(),0);
    GTEST_ASSERT_EQ(pushConstants->size(),32);

    GTEST_ASSERT_EQ(pushConstants->child(0).type(),GraphicsType::VECTOR2);
    GTEST_ASSERT_EQ(pushConstants->child(0).offset(),0);
    GTEST_ASSERT_EQ(pushConstants->child(0).absoluteOffset(),0);
    GTEST_ASSERT_EQ(pushConstants->child(0).size(),8);

    GTEST_ASSERT_EQ(pushConstants->child(1).type(),GraphicsType::VECTOR2);
    GTEST_ASSERT_EQ(pushConstants->child(1).offset(),8);
    GTEST_ASSERT_EQ(pushConstants->child(1).absoluteOffset(),8);
    GTEST_ASSERT_EQ(pushConstants->child(1).size(),8);

    GTEST_ASSERT_EQ(pushConstants->child(2).type(),GraphicsType::VECTOR4);
    GTEST_ASSERT_EQ(pushConstants->child(2).offset(),16);
    GTEST_ASSERT_EQ(pushConstants->child(2).absoluteOffset(),16);
    GTEST_ASSERT_EQ(pushConstants->child(2).size(),16);
}

TEST_F(ShaderPipelineTest, TextureTypes)
{
    ShaderFile stages[] =
    {
     {
            .pathIndicator = "resources/shaders/TextureTypes.vertex",
            .stage = ShaderStageFlags::VERTEX,
        },
     {
        .pathIndicator = "resources/shaders/TextureTypes.fragment",
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
    auto group = pipeline->descriptorGroup(0);
    //these names are a tad confusing. XXXArray means the array is coming from the texture itself, but isn't arrayed in the code
    auto texture1D = group->descriptor(1);
    auto texture1DArray = group->descriptor(2);
    auto texture2D = group->descriptor(3);
    auto texture2DArray = group->descriptor(4);
    auto texture3D = group->descriptor(5);
    auto textureCube = group->descriptor(6);
    auto textureCubeArray = group->descriptor(7);

    GTEST_ASSERT_EQ(texture1D.shape().type, Descriptor::Type::SAMPLED_TEXTURE);
    GTEST_ASSERT_EQ(texture1D.shape().dimension,Descriptor::Dimension::ONE_DIMENSIONAL);
    GTEST_ASSERT_EQ(texture1D.shape().arrayDepth,3);

    GTEST_ASSERT_EQ(texture1DArray.shape().type, Descriptor::Type::SAMPLED_TEXTURE);
    GTEST_ASSERT_EQ(texture1DArray.shape().dimension,Descriptor::Dimension::ONE_DIMENSIONAL);
    GTEST_ASSERT_EQ(texture1DArray.shape().arrayDepth,1);

    GTEST_ASSERT_EQ(texture2D.shape().type, Descriptor::Type::SAMPLED_TEXTURE);
    GTEST_ASSERT_EQ(texture2D.shape().dimension,Descriptor::Dimension::TWO_DIMENSIONAL);
    GTEST_ASSERT_EQ(texture2D.shape().arrayDepth,3);

    GTEST_ASSERT_EQ(texture2DArray.shape().type, Descriptor::Type::SAMPLED_TEXTURE);
    GTEST_ASSERT_EQ(texture2DArray.shape().dimension,Descriptor::Dimension::TWO_DIMENSIONAL);
    GTEST_ASSERT_EQ(texture2DArray.shape().arrayDepth,1);

    GTEST_ASSERT_EQ(texture3D.shape().type, Descriptor::Type::SAMPLED_TEXTURE);
    GTEST_ASSERT_EQ(texture3D.shape().dimension,Descriptor::Dimension::THREE_DIMENSIONAL);
    GTEST_ASSERT_EQ(texture3D.shape().arrayDepth,3);

    GTEST_ASSERT_EQ(textureCube.shape().type, Descriptor::Type::SAMPLED_TEXTURE);
    GTEST_ASSERT_EQ(textureCube.shape().dimension,Descriptor::Dimension::CUBE);
    GTEST_ASSERT_EQ(textureCube.shape().arrayDepth,3);

    GTEST_ASSERT_EQ(textureCubeArray.shape().type, Descriptor::Type::SAMPLED_TEXTURE);
    GTEST_ASSERT_EQ(textureCubeArray.shape().dimension,Descriptor::Dimension::CUBE);
    GTEST_ASSERT_EQ(textureCubeArray.shape().arrayDepth,1);

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
        auto desc = group0->descriptor(i);
        GTEST_ASSERT_TRUE(desc.shape().type == Descriptor::Type::STORAGE_BUFFER);
    }

    ShaderFile computeDrawFile{.pathIndicator = "resources/shaders/ComputeDraw", .stage = ShaderStageFlags::COMPUTE};
    auto computeDraw = GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(computeDrawFile);
    GTEST_ASSERT_EQ(computeDraw->descriptorGroupCount(),1);
    group0 = computeDraw->descriptorGroup(0);
    GTEST_ASSERT_EQ(group0->descriptorCount(),1);
    auto desc = group0->descriptor(0);
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

#ifdef SLAG_DEBUG
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
#endif

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

    testProperties(properties,properties2,cameraTransform,cameraProjection,object1,object2,"resources/textures/depth-clamp-result.png",.99,.5);

}

TEST_F(ShaderPipelineTest,RasterizationDiscard)
{
    //"I'm not smart enough to test this, I've read what it does, and... I don't know how to capture the output? I guess it could be done with a generic write buffer as a descriptor group item?"
    GTEST_SKIP();
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

    testProperties(properties,properties2,cameraTransform,cameraProjection,object1,object2,"resources/textures/rasterizer-discard-result.png",.99,.5);
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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/draw-face-result.png",.99,.5);
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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/cull-none-result.png",.98,.4);
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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/cull-front-result.png",.99,.5);
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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/cull-back-result.png",.99,.5);
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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/front-face-clockwise-result.png",.99,.5);
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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/front-face-counter-clockwise-result.png",.99,.5);
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

    testProperties(properties,properties2,cameraTransform,cameraProjection,object1,object2,"resources/textures/depth-bias-result.png",.99,.5);
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
    testProperties(properties,properties2,cameraTransform,cameraProjection,object1,object2,"resources/textures/depth-bias-slope-result.png",.99,.7);
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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/blend-disabled-result.png",.95,.5);
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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/draw-face-result.png",.99,.5);

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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/color-blend-minus-result.png",.99,.5);

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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/alpha-blend-minus.png",.99,.5);

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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/color-blend-max-result.png",.99,.5);

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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/alpha-blend-max-result.png",.99,.5);

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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/color-blend-min-result.png",.99,.5);

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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/alpha-blend-min-result.png",.99,.5);

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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/color-blend-reverse-minus-result.png",.99,.5);

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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/alpha-blend-reverse-minus-result.png",.99,.5);
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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/blend-color-write-mask-result.png",.99,.5);
}

TEST_F(ShaderPipelineTest,BlendStateBlendFactors)
{
    //TODO: There's too many combinations to test, the combinatrics easily explode to unimaginable numbers, I think maybe I should just test a few? There are 96 tests to do even if I do them one at a time, and I don't think even that is practical. (even small images take up some space)
    GTEST_SKIP();
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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-clear-result.png",.99,.5);
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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-inverted-result.png",.99,.5);
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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-equivalent-result.png",.99,.5);
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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-or-result.png",.99,.5);
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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-and-result.png",.99,.5);
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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-and-inverted-result.png",.99,.5);
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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-and-reverse-result.png",.99,.5);
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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-copy-result.png",.99,.5);
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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-invert-result.png",.99,.5);
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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-nand-result.png",.99,.5);
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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-no-op-result.png",.99,.5);
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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-nor-result.png",.99,.5);
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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-or-inverted-result.png",.99,.5);
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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-reverse-result.png",.99,.5);
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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-set-result.png",.99,.5);
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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/logic-operation-xor-result.png",.99,.5);
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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/draw-face-result.png",.99,.5);
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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/draw-face-result.png",.99,.5);
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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/cull-back-result.png",.99,.7);
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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/draw-face-result.png",.99,.7);
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

    testProperties(properties,properties,cameraTransform,cameraProjection,object1,object2,"resources/textures/draw-face-result.png",.99,.7);
}

TEST_F(ShaderPipelineTest,DepthStencilStateStencilDetails)
{
    //TODO: There's too many combinations to test
    GTEST_SKIP();
}

DescriptorIdentity identifyDescriptor(const DescriptorIdentityParameters& parameters, void* identifyData)
{
    DescriptorIdentity identity{};
    utilities::DescriptorDictionary* dictionary = (utilities::DescriptorDictionary*)identifyData;
    auto data = dictionary->getEntry(parameters.descriptorGroupIndex,parameters.descriptor->name());
    identity.name = data.name;
    identity.index = data.index;
    return identity;

}
TEST_F(ShaderPipelineTest,DescriptorReorder)
{
    ShaderFile files[]=
    {
        ShaderFile("resources/shaders/TexturedDepth.vertex",ShaderStageFlags::VERTEX),
        ShaderFile("resources/shaders/TexturedDepth.fragment",ShaderStageFlags::FRAGMENT)
    };

    FrameBufferDescription framebufferDescription;
    framebufferDescription.colorTargets[0] = Pixels::Format::R8G8B8A8_UNORM;
    framebufferDescription.depthTarget = Pixels::Format::D32_FLOAT;

    ShaderProperties properties{};

    auto shaderOriginal = GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(files,2,properties,vertexPosUVDescription,framebufferDescription);
    auto dictionary = GraphicsAPIEnvironment::graphicsAPIEnvironment()->getShaderDictionary("TexturedDepth");
    auto shaderModified = GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(files,2,properties,vertexPosUVDescription,framebufferDescription,identifyDescriptor,dictionary);

    GTEST_ASSERT_EQ(shaderOriginal->descriptorGroupCount(),shaderModified->descriptorGroupCount());
    for (auto i=0; i< shaderOriginal->descriptorGroupCount(); i++)
    {
        auto groupOriginal = shaderOriginal->descriptorGroup(i);
        auto groupModified = shaderModified->descriptorGroup(i);
        GTEST_ASSERT_EQ(groupOriginal->descriptorCount(), groupModified->descriptorCount());
        GTEST_ASSERT_TRUE(groupOriginal->compatible(groupModified));
    }
    auto groupOriginal = shaderOriginal->descriptorGroup(2);
    auto groupModified = shaderModified->descriptorGroup(2);
    GTEST_ASSERT_EQ(groupOriginal->descriptor(0).shape(), groupModified->descriptor(1).shape());
    GTEST_ASSERT_EQ(groupOriginal->descriptor(1).shape(), groupModified->descriptor(0).shape());
    auto originalLayout = shaderOriginal->uniformBufferLayout(2,0);
    auto modifiedLayout = shaderModified->uniformBufferLayout(2,1);
    GTEST_ASSERT_TRUE(modifiedLayout != nullptr);
    GTEST_ASSERT_EQ(originalLayout->childrenCount(),modifiedLayout->childrenCount());
    GTEST_ASSERT_TRUE(BufferLayout::compatible(*originalLayout,*modifiedLayout));

    GTEST_ASSERT_EQ(groupOriginal->descriptorByteOffset(0), groupModified->descriptorByteOffset(1));
    GTEST_ASSERT_EQ(groupOriginal->descriptorByteOffset(1),groupModified->descriptorByteOffset(0));
}

TEST_F(ShaderPipelineTest, VertexBufferLayouts)
{
    ShaderFile stages[] =
    {
        {
            .pathIndicator = "resources/shaders/CombinedVertex.vertex",
            .stage = ShaderStageFlags::VERTEX,
        },
     {
         .pathIndicator = "resources/shaders/CombinedVertex.fragment",
         .stage = ShaderStageFlags::FRAGMENT,
         }
    };
    ShaderProperties properties{};
    VertexDescription vertexDescription(3);
    vertexDescription.add(GraphicsType::VECTOR3,0,0);
    vertexDescription.add(GraphicsType::VECTOR2,0,1);
    vertexDescription.add(GraphicsType::UNSIGNED_INTEGER,0,2,4);
    vertexDescription.add(GraphicsType::FLOAT,32*4,2,4);
    FrameBufferDescription frameBufferDescription;
    frameBufferDescription.colorTargets[0] = Pixels::Format::R8G8B8A8_UNORM;
    frameBufferDescription.depthTarget = Pixels::Format::D32_FLOAT;


    auto pipeline = GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(stages,2,properties,vertexDescription,frameBufferDescription);
    GTEST_ASSERT_EQ(pipeline->pushConstants(),nullptr);
    GTEST_FAIL();
}