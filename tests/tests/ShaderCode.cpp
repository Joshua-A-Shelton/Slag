#include <gtest/gtest.h>
#include <slag/Slag.h>
#include "../utilities/PlatformUtilities.h"
using namespace slag;

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

TEST(ShaderCode, DescriptorTypes)
{
    GTEST_FAIL();
}