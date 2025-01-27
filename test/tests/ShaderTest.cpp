#include "gtest/gtest.h"
#include "slag/SlagLib.h"
#include <glm/glm.hpp>
using namespace slag;

TEST(Shader, VertexDescription)
{
    struct SimpleVertex
    {
        glm::vec3 position;
        glm::vec2 uv;
    };
    auto vertexStride = sizeof(SimpleVertex);
    std::vector<ShaderModule> modules;
    modules.emplace_back(ShaderStageFlags::VERTEX,"resources/basic.vert.spv");
    modules.emplace_back(ShaderStageFlags::FRAGMENT, "resources/basic.frag.spv");
    ShaderProperties properties{};
    VertexDescription vertexDescription(1);
    vertexDescription.add(GraphicsTypes::VECTOR3, offsetof(SimpleVertex,position),0);
    vertexDescription.add(GraphicsTypes::VECTOR2, offsetof(SimpleVertex,uv),0);
    FrameBufferDescription frameBufferDescription;
    frameBufferDescription.addColorTarget(Pixels::R8G8B8A8_UNORM);
    frameBufferDescription.setDepthTarget(Pixels::D32_FLOAT);
    std::unique_ptr<ShaderPipeline> basicShader = std::unique_ptr<ShaderPipeline>(
            ShaderPipeline::newShaderPipeline(modules.data(), modules.size(), nullptr, 0, properties, &vertexDescription, frameBufferDescription));
    std::unique_ptr<ShaderPipeline> reflectedShader = std::unique_ptr<ShaderPipeline>(
            ShaderPipeline::newShaderPipeline(modules.data(), modules.size(), nullptr, 0, properties, nullptr, frameBufferDescription));

    //actually test something
    GTEST_FAIL();
}