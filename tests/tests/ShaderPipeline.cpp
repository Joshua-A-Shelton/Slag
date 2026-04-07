#include <gtest/gtest.h>
#include <slag/Slag.h>
#include "../utilities/PlatformUtilities.h"
using namespace slag;

TEST(ShaderPipeline, Sandbox)
{
    auto card = Slag::backend()->graphicsCard(0);
    auto vertex = slagTestsCreateShaderModule(card, "resources/tests/shaders/compiled/TexturedDepth.vertex");
    auto fragment = slagTestsCreateShaderModule(card, "resources/tests/shaders/compiled/TexturedDepth.fragment");
    std::vector<VertexBinding> vertexBindings =
    {
        VertexBinding(0,sizeof(float)*5,std::vector<VertexAttribute>{VertexAttribute("POSITION",PixelFormat::R32G32B32_FLOAT,0),VertexAttribute("UV_COORDINATES",PixelFormat::R32G32_FLOAT,sizeof(float)*3)})
    };
    VertexDescription vertexDesc(vertexBindings);
    FramebufferDescription framebufferDesc{};
    framebufferDesc.colorFormats[0] = PixelFormat::R8G8B8A8_UNORM;
    framebufferDesc.depthFormat = PixelFormat::D32_FLOAT;
    auto pipeline = std::unique_ptr<ShaderPipeline>(card->newShaderPipeline(vertexDesc,vertex.get(), fragment.get(),PipelineState{},framebufferDesc));
    int i=0;
    GTEST_FAIL();
}

TEST(ShaderPipeline, OverlappingBindGroups)
{
    //Need to test a bind group where some / all descriptors are used on multiple stages
    GTEST_FAIL();
}

TEST(ShaderPipeline, IncompatibleBindGroups)
{
    GTEST_FAIL();
}