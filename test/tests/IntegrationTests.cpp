#include "gtest/gtest.h"
#include "slag/SlagLib.h"
#include <glm/glm.hpp>
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
        std::vector<Vertex> tverts = {{{0,1,1},{.5,0}},{{1,1,1},{1,1}},{{-1,1,1},{0,1}}};
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
    /*auto frameBuffer = std::unique_ptr<Texture>(Texture::newTexture(Pixels::R8G8B8A8_UNORM,slag::Texture::TEXTURE_2D,500,500,1,1,1,TextureUsageFlags::RENDER_TARGET_ATTACHMENT));
    ShaderModule modules[2] = {ShaderModule(ShaderStageFlags::VERTEX,"resources\\integration\\flat.frag.spv"),ShaderModule(ShaderStageFlags::FRAGMENT,"resources\\integration\\flat.vert.spv")};
    ShaderProperties shaderProps{};
    FrameBufferDescription description;
    description.addColorTarget(Pixels::R8G8B8A8_UNORM);
    auto shader = std::unique_ptr<Shader>(Shader::newShader(modules,2, nullptr,0,shaderProps,&vertexPosUVDescription,description));
    auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GpuQueue::GRAPHICS));
    auto descriptorPool = std::unique_ptr<DescriptorPool>(DescriptorPool)
    commandBuffer->begin();
    commandBuffer->bindDescriptorPool()*/

}