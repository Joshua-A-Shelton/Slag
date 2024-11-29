#include "gtest/gtest.h"
#include "slag/SlagLib.h"
#include <glm/glm.hpp>
#include <lodepng.h>

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
    GTEST_FAIL();

}