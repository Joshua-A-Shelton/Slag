#include <gtest/gtest.h>
#include <slag/Slag.h>
#include "../GraphicsAPIEnvironment.h"
using namespace slag;
TEST(ShaderPipeline, DescriptorGroupReflection)
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
    GTEST_ASSERT_EQ(group1->descriptor(0).shape().binding,1);

    GTEST_ASSERT_TRUE(group1->descriptor(1).shape().type == Descriptor::Type::SAMPLER_AND_TEXTURE);
    GTEST_ASSERT_EQ(group1->descriptor(1).shape().arrayDepth,1);
    GTEST_ASSERT_EQ(group1->descriptor(1).shape().binding,0);

    GTEST_ASSERT_TRUE(group2->descriptor(0).shape().type == Descriptor::Type::UNIFORM_BUFFER);
    GTEST_ASSERT_EQ(group2->descriptor(0).shape().arrayDepth,1);
    GTEST_ASSERT_EQ(group2->descriptor(0).shape().binding,0);

    auto layout0_0 = pipeline->uniformBufferLayout(0,0);
    auto layout1_0 = pipeline->uniformBufferLayout(1,0);
    auto layout1_1 = pipeline->uniformBufferLayout(1,1);
    auto layout2_0 = pipeline->uniformBufferLayout(2,1);

    GTEST_FAIL();
}