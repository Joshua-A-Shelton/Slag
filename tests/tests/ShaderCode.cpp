#include <gtest/gtest.h>
#include <slag/Slag.h>
#include "../utilities/PlatformUtilities.h"
using namespace slag;

//Some compilers (like slangc) may treat matrices as structs of 4 vector4s, we'll allow it
bool is4x4MatrixType(const StructMember& member)
{
    if (member.type()==GraphicsType::MATRIX_4X4)
    {
        return true;
    }
    else if (member.type()==GraphicsType::STRUCT)
    {
        if (member.childCount() == 1)
        {
            auto& child = member.child(0);
            if (child.type() == GraphicsType::VECTOR4 && child.arrayLength()==4)
            {
                return true;
            }
        }
        else if (member.childCount()==4)
        {
            bool uniformChildren = true;
            for (auto i=0; i < member.childCount(); i++)
            {
                if (member.child(i).type() != GraphicsType::VECTOR4)
                {
                    uniformChildren = false;
                }
            }
            return uniformChildren;
        }
    }
    return false;
}

TEST(ShaderCode, Reflection)
{
    auto card = Slag::backend()->graphicsCard(0);

    auto shaderModule = slagTestsCreateShaderModule(card, "resources/tests/shaders/compiled/UnlitTextured.vertex");
    GTEST_ASSERT_TRUE(shaderModule->metaData().type() == ShaderType::VERTEX);
    GTEST_ASSERT_TRUE(shaderModule->metaData().bindGroupCount() == 2);

    auto bindGroup0 = shaderModule->metaData().bindGroup(0);
    auto bindGroup1 = shaderModule->metaData().bindGroup(1);


    GTEST_ASSERT_TRUE(bindGroup0.bindIndex() == 0);
    GTEST_ASSERT_TRUE(bindGroup0.descriptorInfoCount() == 1);
    auto& descInfo = bindGroup0.descriptorInfo(0);
    GTEST_ASSERT_TRUE(descInfo.type() == DescriptorType::UNIFORM_BUFFER);
    GTEST_ASSERT_TRUE(descInfo.bufferDetails()->memberCount() == 3);
    for (int i=0; i < descInfo.bufferDetails()->memberCount(); i++)
    {
        GTEST_ASSERT_TRUE(is4x4MatrixType(descInfo.bufferDetails()->member(i)));
    }

    auto binding1 = shaderModule->metaData().bindGroup(1);
    GTEST_ASSERT_TRUE(binding1.bindIndex() == 3);
    GTEST_ASSERT_TRUE(binding1.descriptorInfoCount() == 1);
    auto& descInfo1 = binding1.descriptorInfo(0);
    GTEST_ASSERT_TRUE(descInfo1.type() == DescriptorType::UNIFORM_BUFFER);
    GTEST_ASSERT_TRUE(descInfo1.bufferDetails()->memberCount() == 1);
    GTEST_ASSERT_TRUE(is4x4MatrixType(descInfo1.bufferDetails()->member(0)));

    GTEST_FAIL();
}