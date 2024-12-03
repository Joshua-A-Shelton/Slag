#include "gtest/gtest.h"
#include "slag/SlagLib.h"
#include <glm/glm.hpp>
#include <lodepng.h>
#include "../utils/Window.h"

using namespace slag;
class IntegrationTests: public ::testing::Test
{
protected:
    std::unique_ptr<Buffer> triangleVerts = nullptr;
    std::unique_ptr<Buffer> triangleIndicies = nullptr;
    std::unique_ptr<Buffer> triangleNormals = nullptr;
    std::unique_ptr<Buffer> cubeVerts = nullptr;
    std::unique_ptr<Buffer> cubeIndicies = nullptr;
    std::unique_ptr<Buffer> cubeNormals = nullptr;
    VertexDescription vertexPosUVDescription = VertexDescription(1);
    VertexDescription vertexPosUVNormalDescription = VertexDescription(2);
    struct Vertex
    {
        glm::vec3 position;
        glm::vec2 uv;
    };
public:
    IntegrationTests()
    {
        std::vector<Vertex> tverts = {{{1.f, 1.f, 0.0f},{.5,0}},{{ -1.f, 1.f, 0.0f},{1,1}},{{0.f,-1.f, 0.0f},{0,1}}};
        std::vector<uint16_t> tindexes = {0,1,2};
        std::vector<glm::vec3> tnormals = {{0,0,1},{0,0,1},{0,0,1}};
        triangleVerts = std::unique_ptr<Buffer>(Buffer::newBuffer(tverts.data(),tverts.size()*sizeof(Vertex),Buffer::GPU,Buffer::VERTEX_BUFFER));
        triangleIndicies = std::unique_ptr<Buffer>(Buffer::newBuffer(tindexes.data(),tindexes.size()*sizeof(uint16_t),Buffer::GPU,Buffer::INDEX_BUFFER));
        triangleNormals = std::unique_ptr<Buffer>(Buffer::newBuffer(tnormals.data(),tverts.size()*sizeof(glm::vec3 ),Buffer::GPU,Buffer::VERTEX_BUFFER));
        vertexPosUVDescription.add(GraphicsTypes::VECTOR3, offsetof(Vertex,position),0).add(GraphicsTypes::VECTOR2, offsetof(Vertex,uv),0);
        vertexPosUVNormalDescription.add(GraphicsTypes::VECTOR3, offsetof(Vertex,position),0).add(GraphicsTypes::VECTOR2, offsetof(Vertex,uv),0).add(GraphicsTypes::VECTOR3,0,1);
    }
    virtual ~IntegrationTests() {}
    virtual void SetUp() {}
    virtual void TearDown() {}
};
TEST_F(IntegrationTests, BasicTriangle)
{

    ShaderModule modules[2] = {ShaderModule(ShaderStageFlags::VERTEX,"resources/shaders/orange.vert.spv"),ShaderModule(ShaderStageFlags::FRAGMENT,"resources/shaders/orange.frag.spv")};
    ShaderProperties props;
    FrameBufferDescription description;
    description.addColorTarget(Pixels::B8G8R8A8_UNORM_SRGB);
    auto shader = std::unique_ptr<Shader>(Shader::newShader(modules,2, nullptr,0,props, nullptr,description));
    auto window = slag::Window::makeWindow("Integration::BasicTriangle",500,500);
    auto swapchain = slag::Window::makeSwapchain(window.get(),3,Swapchain::PresentMode::MAILBOX,Pixels::B8G8R8A8_UNORM_SRGB);
    auto texture = std::unique_ptr<Texture>(Texture::newTexture("resources/test-img.png",Pixels::R8G8B8A8_UNORM,1,TextureUsageFlags::SAMPLED_IMAGE,Texture::SHADER_RESOURCE));
    SamplerBuilder sb{};
    auto sampler = std::unique_ptr<Sampler>(sb.newSampler());
    size_t count = 0;
    auto verts = triangleVerts.get();
    size_t offset = 0;
    auto indexes = triangleIndicies.get();

    while(count < 10000)
    {
        if(auto frame = swapchain->next())
        {
            auto backBuffer = frame->backBuffer();
            auto commandBuffer = frame->commandBuffer();
            commandBuffer->begin();
            commandBuffer->setViewPort(0,0,backBuffer->width(),backBuffer->height(),1,0);
            commandBuffer->setScissors({{0,0},{backBuffer->width(),backBuffer->height()}});
            ImageBarrier toRT{.texture=backBuffer,.oldLayout=slag::Texture::UNDEFINED,.newLayout=Texture::RENDER_TARGET,.accessBefore=BarrierAccessFlags::NONE,.accessAfter=BarrierAccessFlags::SHADER_WRITE,.syncBefore=PipelineStageFlags::NONE,.syncAfter=PipelineStageFlags::ALL_GRAPHICS};
            commandBuffer->insertBarriers(&toRT,1, nullptr,0, nullptr,0);
            ClearValue cv{};
            cv.color = ClearColor{.floats={0,.6,.7,1}};
            Attachment attachment(backBuffer,Texture::RENDER_TARGET, true,cv);
            commandBuffer->beginRendering(&attachment,1, nullptr,slag::Rectangle{{0,0},{backBuffer->width(),backBuffer->height()}});
            commandBuffer->bindGraphicsShader(shader.get());
            commandBuffer->bindVertexBuffers(0,&verts,&offset,1);
            commandBuffer->bindIndexBuffer(indexes,Buffer::UINT16,0);
            commandBuffer->drawIndexed(3,1,0,0,0);
            commandBuffer->endRendering();
            ImageBarrier toPresent{.texture=backBuffer,.oldLayout=slag::Texture::RENDER_TARGET,.newLayout=Texture::PRESENT,.accessBefore=BarrierAccessFlags::SHADER_WRITE,.accessAfter=BarrierAccessFlags::ALL_READ,.syncBefore=PipelineStageFlags::ALL_GRAPHICS,.syncAfter=PipelineStageFlags::NONE};
            commandBuffer->insertBarriers(&toPresent,1, nullptr,0, nullptr,0);
            commandBuffer->end();
            SlagLib::graphicsCard()->graphicsQueue()->submit(&commandBuffer,1, nullptr,0, nullptr,0,frame);
            commandBuffer->waitUntilFinished();
        }
        count++;
    }

    GTEST_FAIL();
}