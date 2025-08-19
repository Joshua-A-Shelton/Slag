#include <gtest/gtest.h>
#include <slag/Slag.h>
#include "../GraphicsAPIEnvironment.h"
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

TEST(ShaderPipeline, DescriptorGroupReflectionCompute)
{
    GTEST_FAIL();
}

TEST(ShaderPipeline, MultiStageFlagFail)
{
    GTEST_FAIL();
}

TEST(ShaderPipeline, DepthClamp)
{
    GTEST_FAIL();
}
TEST(ShaderPipeline,RasterizationDiscard)
{
    GTEST_FAIL();
}
TEST(ShaderPipeline,DrawFace)
{
    GTEST_FAIL();
}
TEST(ShaderPipeline,DrawEdges)
{
    GTEST_FAIL();
}
TEST(ShaderPipeline,DrawVerticies)
{
    GTEST_FAIL();
}
TEST(ShaderPipeline,DrawThicknessEdges)
{
    GTEST_FAIL();
}
TEST(ShaderPipeline,DrawThicknessVerticies)
{
    GTEST_FAIL();
}
TEST(ShaderPipeline,CullNone)
{
    GTEST_FAIL();
}
TEST(ShaderPipeline,CullFront)
{
    GTEST_FAIL();
}
TEST(ShaderPipeline,CullBack)
{
    GTEST_FAIL();
}
TEST(ShaderPipeline,FrontFaceClockWise)
{
    GTEST_FAIL();
}
TEST(ShaderPipeline,FrontFaceCounterClockWise)
{
    GTEST_FAIL();
}
TEST(ShaderPipeline,DepthBias)
{
    GTEST_FAIL();
}
TEST(ShaderPipeline,MultiSample)
{
    GTEST_FAIL();
}
TEST(ShaderPipeline,MultiSampleAlpha)
{
    GTEST_FAIL();
}

TEST(ShaderPipeline,BlendState)
{
    GTEST_FAIL();
}

TEST(ShaderPipeline,StencilState)
{
    GTEST_FAIL();
}