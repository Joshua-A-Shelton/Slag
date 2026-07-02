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


TEST(ShaderCode, VertexShader)
{
    auto card = Slag::backend()->graphicsCard(0);

    auto shaderModule = utilities::createShaderModule(card, "resources/tests/shaders/compiled/TexturedDepth.vertex");
    GTEST_FAIL();
}

TEST(ShaderCode, GeometryShader)
{
    GTEST_FAIL();
}

TEST(ShaderCode, FragmentShader)
{
    auto card = Slag::backend()->graphicsCard(0);

    auto shaderModule = utilities::createShaderModule(card, "resources/tests/shaders/compiled/TexturedDepth.fragment");
    GTEST_FAIL();
}

TEST(ShaderCode, ComputeShader)
{
    GTEST_FAIL();
}

TEST(ShaderCode, RayGenShader)
{
    GTEST_FAIL();
}

TEST(ShaderCode, AnyHitShader)
{
    GTEST_FAIL();
}

TEST(ShaderCode, ClosestHitShader)
{
    GTEST_FAIL();
}

TEST(ShaderCode, MissShader)
{
    GTEST_FAIL();
}

TEST(ShaderCode, IntersectionShader)
{
    GTEST_FAIL();
}

TEST(ShaderCode, CallableShader)
{
    GTEST_FAIL();
}

TEST(ShaderCode, MeshShader)
{
    GTEST_FAIL();
}

TEST(ShaderCode, TaskShader)
{
    GTEST_FAIL();
}

TEST(ShaderCode, BufferLayoutReflection)
{
    GTEST_FAIL();
}

TEST(ShaderCode, TexelBufferReflection)
{
    GTEST_FAIL();
}

TEST(ShaderCode, TextureTypeReflection)
{
    GTEST_FAIL();
}

TEST(ShaderCode, SamplerReflection)
{
    GTEST_FAIL();
}

TEST(ShaderCode, AccelerationStructureReflection)
{
    GTEST_FAIL();
}