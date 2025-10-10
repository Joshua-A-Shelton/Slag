#include <gtest/gtest.h>
#include <slag/Slag.h>
#include "../GraphicsAPIEnvironment.h"
using namespace slag;
TEST(DescriptorPool, DefaultInstantiation)
{
    auto pool = std::unique_ptr<DescriptorPool>(DescriptorPool::newDescriptorPool());
    GTEST_ASSERT_NE(pool.get(), nullptr);
}
TEST(DescriptorPool, ParameterizedInstantiation)
{
    auto pool = std::unique_ptr<DescriptorPool>(DescriptorPool::newDescriptorPool(DescriptorPoolPageInfo{}));
    GTEST_ASSERT_NE(pool.get(), nullptr);
}
TEST(DescriptorPool, MakeBundle)
{
    auto pool = std::unique_ptr<DescriptorPool>(DescriptorPool::newDescriptorPool());
    ShaderFile files[]=
    {
        ShaderFile("resources/shaders/TexturedDepth.vertex",ShaderStageFlags::VERTEX),
        ShaderFile("resources/shaders/TexturedDepth.fragment",ShaderStageFlags::FRAGMENT)
    };
    FrameBufferDescription framebufferDescription;
    framebufferDescription.colorTargets[0] = Pixels::Format::R8G8B8A8_UNORM;
    framebufferDescription.depthTarget = Pixels::Format::D32_FLOAT_S8X24_UINT;
    ShaderProperties properties{};
    VertexDescription vertexDescription(2);
    vertexDescription.add(GraphicsType::VECTOR3,0,0).add(GraphicsType::VECTOR2,0,1);
    auto shader = GraphicsAPIEnvironment::graphicsAPIEnvironment()->loadPipelineFromFiles(files,2,properties,vertexDescription,framebufferDescription);

    auto bundle = pool->makeBundle(shader->descriptorGroup(0));
}
TEST(DescriptorPool, Reset)
{
    auto pool = std::unique_ptr<DescriptorPool>(DescriptorPool::newDescriptorPool());
    pool->reset();
}