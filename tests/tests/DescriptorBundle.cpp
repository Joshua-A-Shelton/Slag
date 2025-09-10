#include <gtest/gtest.h>
#include <slag/Slag.h>
#include "../GraphicsAPIEnvironment.h"
using namespace slag;

class DescriptorBundleTest : public ::testing::Test
{
public:
    std::unique_ptr<ShaderPipeline> pipeline;
    std::unique_ptr<DescriptorPool> descriptorPool;
    DescriptorBundleTest()
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


        pipeline = GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(stages, 2, properties, vertexDescription, frameBufferDescription);
        descriptorPool = std::unique_ptr<DescriptorPool>(DescriptorPool::newDescriptorPool());
    }
};
#ifndef SLAG_DISCREET_TEXTURE_LAYOUTS

TEST_F(DescriptorBundleTest, SetSamplerAllTypesPipeline)
{
    descriptorPool->reset();
    auto bundle1 = descriptorPool->makeBundle(pipeline->descriptorGroup(0));
    std::unique_ptr<Sampler> sampler = std::unique_ptr<Sampler>(Sampler::newSampler(SamplerParameters{}));
    bundle1.setSampler(1,0,sampler.get());
}

TEST_F(DescriptorBundleTest, setSampledTextureAllTypesPipeline)
{
    descriptorPool->reset();
    auto bundle1 = descriptorPool->makeBundle(pipeline->descriptorGroup(0));
    auto texture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UINT,Texture::Type::TEXTURE_2D,Texture::UsageFlags::SAMPLED_IMAGE,32,32,1,1));
    bundle1.setSampledTexture(2,0,texture.get());
}

TEST_F(DescriptorBundleTest, setStorageTextureAllTypesPipeline)
{
    descriptorPool->reset();
    auto bundle1 = descriptorPool->makeBundle(pipeline->descriptorGroup(0));
    auto texture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UINT,Texture::Type::TEXTURE_2D,Texture::UsageFlags::STORAGE,32,32,1,1));
    bundle1.setStorageTexture(3,0,texture.get());
}

#else
TEST_F(DescriptorBundleTest, SetSamplerAllTypesPipeline)
{
    GTEST_FAIL();
}

TEST_F(DescriptorBundleTest, setSampledTextureAllTypesPipeline)
{
    GTEST_FAIL();
}

TEST_F(DescriptorBundleTest, setStorageTextureAllTypesPipeline)
{
    GTEST_FAIL();
}

#endif

TEST_F(DescriptorBundleTest, SetUniformTexelBufferAllTypesPipeline)
{
    descriptorPool->reset();
    auto bundle2 = descriptorPool->makeBundle(pipeline->descriptorGroup(1));
    auto texelBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(256,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_TEXEL_BUFFER));
    auto bufferView = std::unique_ptr<BufferView>(BufferView::newBufferView(texelBuffer.get(),pipeline->texelBufferDescription(1,0)->format(),0,texelBuffer->size()));
    bundle2.setUniformTexelBuffer(0,0,bufferView.get());
}

TEST_F(DescriptorBundleTest, SetStorageTexelBufferAllTypesPipeline)
{
    descriptorPool->reset();
    auto bundle2 = descriptorPool->makeBundle(pipeline->descriptorGroup(1));
    auto texelBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(256,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::STORAGE_TEXEL_BUFFER));
    auto bufferView = std::unique_ptr<BufferView>(BufferView::newBufferView(texelBuffer.get(),pipeline->texelBufferDescription(1,1)->format(),0,texelBuffer->size()));

    bundle2.setStorageTexelBuffer(1,0,bufferView.get());

    auto texelBuffer2 = std::unique_ptr<Buffer>(Buffer::newBuffer(256,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::STORAGE_TEXEL_BUFFER));
    bundle2.setStorageTexelBuffer(1,1,bufferView.get());
}

TEST_F(DescriptorBundleTest, SetUniformBufferAllTypesPipeline)
{
    descriptorPool->reset();
    auto bundle1 = descriptorPool->makeBundle(pipeline->descriptorGroup(0));
    auto uniformBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(pipeline->bufferLayout(0,0)->size(),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_BUFFER));
    bundle1.setUniformBuffer(0,0,uniformBuffer.get(),0,uniformBuffer->size());
}

TEST_F(DescriptorBundleTest, SetStorageBufferAllTypesPipeline)
{
    descriptorPool->reset();
    auto bundle2 = descriptorPool->makeBundle(pipeline->descriptorGroup(1));
    auto storageBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(float)+sizeof(int32_t),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::STORAGE_BUFFER));
    bundle2.setStorageBuffer(2,0,storageBuffer.get(),0,storageBuffer->size());
}

TEST_F(DescriptorBundleTest, TexelBufferComputeResults)
{
    descriptorPool->reset();
    ShaderFile stage =
    {
        .pathIndicator = "resources/shaders/SampledAddition",
        .stage = ShaderStageFlags::COMPUTE,
    };
    auto texelCompute = std::unique_ptr<ShaderPipeline>(GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(stage));
    auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GPUQueue::QueueType::COMPUTE));
    auto finished = std::unique_ptr<Semaphore>(Semaphore::newSemaphore(0));
    struct ShaderParams
    {
        uint32_t arrayIndex = 1;
        uint32_t arrayLength = 10;
    };
    ShaderParams shaderParams{};
    auto parameterBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(&shaderParams,sizeof(ShaderParams),Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_BUFFER));
    auto sampler = std::unique_ptr<Sampler>(Sampler::newSampler(SamplerParameters{}));
    std::vector<float> data(40);
    for (auto i=0; i < 40; i++)
    {
        data[i] = 1.0f/(1.0f-((float)i/40.0f));
    }
    auto operands1_1 = std::unique_ptr<Buffer>(Buffer::newBuffer(data.data(),Pixels::size(Pixels::Format::R32G32B32A32_FLOAT)*10,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_TEXEL_BUFFER));
    for (auto i=0; i < 40; i++)
    {
        data[i] = (1.0f/(1.0f-((float)i/80.0f)));
    }
    auto operands1_2 = std::unique_ptr<Buffer>(Buffer::newBuffer(data.data(),Pixels::size(Pixels::Format::R32G32B32A32_FLOAT)*10,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_TEXEL_BUFFER));
    std::vector<uint8_t> data2(40);
    for (auto i=0; i < 40; i++)
    {
        data2[i] = i;
    }
    auto operands2_1 = std::unique_ptr<Buffer>(Buffer::newBuffer(data2.data(),Pixels::size(Pixels::Format::R8G8B8A8_UINT)*10,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_TEXEL_BUFFER));
    for (auto i=0; i < 40; i++)
    {
        data2[i] = i*2;
    }
    auto operands2_2 = std::unique_ptr<Buffer>(Buffer::newBuffer(data2.data(),Pixels::size(Pixels::Format::R8G8B8A8_UINT)*10,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::UNIFORM_TEXEL_BUFFER));

    auto results1_1 = std::unique_ptr<Buffer>(Buffer::newBuffer(Pixels::size(Pixels::Format::R32G32B32A32_FLOAT)*10,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::STORAGE_TEXEL_BUFFER));
    auto results1_2 = std::unique_ptr<Buffer>(Buffer::newBuffer(Pixels::size(Pixels::Format::R32G32B32A32_FLOAT)*10,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::STORAGE_TEXEL_BUFFER));
    auto indexes1_1 =std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(float)*10,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::STORAGE_BUFFER));
    auto indexes1_2 =std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(float)*10,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::STORAGE_BUFFER));

    auto operandView1_1 = std::unique_ptr<BufferView>(BufferView::newBufferView(operands1_1.get(),Pixels::Format::R32G32B32A32_FLOAT,0,operands1_1->size()));
    auto operandView1_2 = std::unique_ptr<BufferView>(BufferView::newBufferView(operands1_2.get(),Pixels::Format::R32G32B32A32_FLOAT,0,operands1_2->size()));

    auto operandView2_1 = std::unique_ptr<BufferView>(BufferView::newBufferView(operands2_1.get(),Pixels::Format::R8G8B8A8_UINT,0,operands2_1->size()));
    auto operandView2_2 = std::unique_ptr<BufferView>(BufferView::newBufferView(operands2_2.get(),Pixels::Format::R8G8B8A8_UINT,0,operands2_2->size()));

    auto resultsView1_1 = std::unique_ptr<BufferView>(BufferView::newBufferView(results1_1.get(),Pixels::Format::R32G32B32A32_FLOAT,0,results1_1->size()));
    auto resultsView1_2 = std::unique_ptr<BufferView>(BufferView::newBufferView(results1_2.get(),Pixels::Format::R32G32B32A32_FLOAT,0,results1_2->size()));

    commandBuffer->begin();

    commandBuffer->bindDescriptorPool(descriptorPool.get());
    commandBuffer->bindComputeShaderPipeline(texelCompute.get());
    auto bundle = descriptorPool->makeBundle(texelCompute->descriptorGroup(0));
    bundle.setUniformBuffer(0,0,parameterBuffer.get(),0,parameterBuffer->size());
    bundle.setSampler(1,0,sampler.get());
    bundle.setUniformTexelBuffer(2,0,operandView1_1.get());
    bundle.setUniformTexelBuffer(2,1,operandView1_2.get());
    bundle.setUniformTexelBuffer(3,0,operandView2_1.get());
    bundle.setUniformTexelBuffer(3,1,operandView2_2.get());
    bundle.setStorageTexelBuffer(4,0,resultsView1_1.get());
    bundle.setStorageTexelBuffer(4,1,resultsView1_2.get());
    bundle.setStorageBuffer(5,0,indexes1_1.get(),0,indexes1_1->size());
    bundle.setStorageBuffer(5,1,indexes1_2.get(),0,indexes1_2->size());
    commandBuffer->bindComputeDescriptorBundle(0,bundle);
    commandBuffer->dispatch(10,1,1);

    commandBuffer->end();

    auto cbptr = commandBuffer.get();
    SemaphoreValue signal
    {
        .semaphore = finished.get(),
        .value = 1
    };

    QueueSubmissionBatch submitInfo
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = &cbptr,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1,
    };
    slagGraphicsCard()->computeQueue()->submit(&submitInfo,1);
    finished->waitForValue(1);

    auto results = results1_2->as<float>();
    auto indexes = indexes1_2->as<float>();

    for (int i=0; i< 10; i++)
    {
        float number1 = (1.0f/(1.0f-((float)i/80.0f)));
        float number2 = (float)(i*2)/255.0f;

        float expected = results[i];
        float index = indexes[i];
        GTEST_ASSERT_EQ(index,i);
        GTEST_ASSERT_EQ(expected,number1+number2);

    }

}
