#include "gtest/gtest.h"
#include "slag/SlagLib.h"
#include <glm/glm.hpp>
#include <lodepng.h>
#include "../utils/Window.h"
#include "stb_image.h"

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
        std::vector<Vertex> tverts = {{{1.f, 1.f, 0.0f},{1,1}},{{ -1.f, 1.f, 0.0f},{0,1}},{{0.f,-1.f, 0.0f},{.5,0}}};
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

    ShaderModule modules[2] = {ShaderModule(ShaderStageFlags::VERTEX,"resources/shaders/flat.vert.spv"),ShaderModule(ShaderStageFlags::FRAGMENT,"resources/shaders/flat.frag.spv")};
    ShaderProperties props;
    FrameBufferDescription description;
    description.addColorTarget(Pixels::R8G8B8A8_UNORM);
    auto shader = std::unique_ptr<Shader>(Shader::newShader(modules,2, nullptr,0,props, nullptr,description));
    auto window = slag::Window::makeWindow("Integration::BasicTriangle",500,500);
    //auto swapchain = slag::Window::makeSwapchain(window.get(),3,Swapchain::PresentMode::MAILBOX,Pixels::B8G8R8A8_UNORM_SRGB);
    auto texture = std::unique_ptr<Texture>(Texture::newTexture("resources/test-img.png",Pixels::R8G8B8A8_UNORM,1,TextureUsageFlags::SAMPLED_IMAGE,Texture::SHADER_RESOURCE));
    auto backBuffer = std::unique_ptr<Texture>(Texture::newTexture(Pixels::R8G8B8A8_UNORM,slag::Texture::TEXTURE_2D,500,500,1,1,1,TextureUsageFlags::RENDER_TARGET_ATTACHMENT));
    SamplerBuilder sb{};
    auto sampler = std::unique_ptr<Sampler>(sb.newSampler());
    size_t count = 0;
    auto verts = triangleVerts.get();
    size_t offset = 0;
    auto indexes = triangleIndicies.get();
    auto pool = std::unique_ptr<DescriptorPool>(DescriptorPool::newDescriptorPool());

    auto data = std::unique_ptr<Buffer>(Buffer::newBuffer(backBuffer->width()*backBuffer->height()*4,Buffer::CPU_AND_GPU,Buffer::DATA_BUFFER));

    auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GpuQueue::GRAPHICS));
    commandBuffer->begin();
    commandBuffer->bindDescriptorPool(pool.get());
    commandBuffer->setViewPort(0,0,backBuffer->width(),backBuffer->height(),1,0);
    commandBuffer->setScissors({{0,0},{backBuffer->width(),backBuffer->height()}});
    ImageBarrier toRT{.texture=backBuffer.get(),.oldLayout=slag::Texture::UNDEFINED,.newLayout=Texture::RENDER_TARGET,.accessBefore=BarrierAccessFlags::NONE,.accessAfter=BarrierAccessFlags::SHADER_WRITE,.syncBefore=PipelineStageFlags::NONE,.syncAfter=PipelineStageFlags::ALL_GRAPHICS};
    commandBuffer->insertBarriers(&toRT,1, nullptr,0, nullptr,0);
    ClearValue cv{};
    cv.color = ClearColor{.floats={0,.6,.7,1}};
    Attachment attachment(backBuffer.get(),Texture::RENDER_TARGET, true,cv);
    commandBuffer->beginRendering(&attachment,1, nullptr,slag::Rectangle{{0,0},{backBuffer->width(),backBuffer->height()}});
    commandBuffer->bindGraphicsShader(shader.get());
    commandBuffer->bindVertexBuffers(0,&verts,&offset,1);
    commandBuffer->bindIndexBuffer(indexes,Buffer::UINT16,0);
    auto bundle = pool->makeBundle(shader->descriptorGroup(0));
    bundle.setSamplerAndTexture(0,0,texture.get(),Texture::SHADER_RESOURCE,sampler.get());
    commandBuffer->bindGraphicsDescriptorBundle(shader.get(),0,bundle);
    commandBuffer->drawIndexed(3,1,0,0,0);
    commandBuffer->endRendering();
    ImageBarrier toCopy{.texture=backBuffer.get(),.oldLayout=slag::Texture::RENDER_TARGET,.newLayout=Texture::TRANSFER_SOURCE,.accessBefore=BarrierAccessFlags::SHADER_WRITE,.accessAfter=BarrierAccessFlags::ALL_READ,.syncBefore=PipelineStageFlags::ALL_GRAPHICS,.syncAfter=PipelineStageFlags::ALL_COMMANDS};
    commandBuffer->insertBarriers(&toCopy,1, nullptr,0, nullptr,0);
    commandBuffer->copyImageToBuffer(backBuffer.get(),Texture::TRANSFER_SOURCE,0,1,0,data.get(),0);
    commandBuffer->end();
    SlagLib::graphicsCard()->graphicsQueue()->submit(commandBuffer.get());
    commandBuffer->waitUntilFinished();

    auto pixelData = data->downloadData();

    int w, h, channels;

    auto rawBytes = stbi_load(std::filesystem::absolute("resources/Integration-BasicTriangle-Render.png").string().c_str(),&w,&h,&channels,4);
    std::vector<std::byte> groundTruth(w*h*4);
    memcpy(groundTruth.data(),rawBytes,w*h*4);
    stbi_image_free(rawBytes);
    GTEST_ASSERT_TRUE(pixelData.size() == groundTruth.size());
    for(size_t i=0; i< pixelData.size(); i++)
    {
        GTEST_ASSERT_EQ(pixelData[i],groundTruth[i]);
    }

}