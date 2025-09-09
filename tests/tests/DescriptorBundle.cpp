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
