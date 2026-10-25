// The smallest Slag program that draws to a window.

#include "SampleApp.h"

#include <cstring>

namespace {

struct Vertex
{
    float x, y;
    float r, g, b;
};

} // namespace

int main(int argc, char** argv)
{
    SampleApp app("Slag triangle", parseOptions(argc, argv));
    if (!app.ok())
    {
        return 1;
    }
    slag::GraphicsDevice* device = app.device;

    SampleShader vertexShader("Triangle", "vertexMain");
    SampleShader pixelShader("Triangle", "pixelMain");
    const slag::VertexAttribute attributes[] = {
        {.location = 0, .format = slag::PixelFormat::R32G32Float, .slot = 0, .offset = 0},
        {.location = 1, .format = slag::PixelFormat::R32G32B32Float, .slot = 0, .offset = 8}};
    const slag::VertexStepRate slots[] = {slag::VertexStepRate::PerVertex};
    const slag::PixelFormat colorFormats[] = {app.colorFormat};
    slag::Pipeline* pipeline = device
                                   ->createGraphicsPipeline({.vertex = vertexShader.stage("vertexMain"),
                                                             .pixel = pixelShader.stage("pixelMain"),
                                                             .vertexInput = {.attributes = attributes, .slots = slots},
                                                             .colorFormats = colorFormats,
                                                             .debugName = "triangle"})
                                   .value();

    // Upload memory is fine for a few vertices read once per frame.
    const Vertex vertices[] = {
        {0.0f, 0.6f, 1, 0.2f, 0.2f}, {0.6f, -0.5f, 0.2f, 1, 0.2f}, {-0.6f, -0.5f, 0.2f, 0.3f, 1}};
    slag::Buffer* vertexBuffer = device
                                     ->createBuffer({.size = sizeof(vertices),
                                                     .placement = slag::Placement::automatic(slag::MemoryKind::Upload)})
                                     .value();
    if (!pipeline || !vertexBuffer)
    {
        return 1;
    }
    std::memcpy(vertexBuffer->mappedData(), vertices, sizeof(vertices));

    int result = app.run([&](slag::CommandBuffer* commandBuffer, slag::AttachmentView* target, uint32_t, uint32_t) {
        const slag::ColorAttachment color = {
            .view = target, .loadOp = slag::LoadOp::Clear, .clearValue = {.color = {0.02f, 0.02f, 0.04f, 1.0f}}};
        commandBuffer->beginRendering({.colorAttachments = {&color, 1}});
        commandBuffer->bindPipeline(pipeline);
        const slag::VertexBufferBinding binding = {vertexBuffer, 0, sizeof(vertices), sizeof(Vertex)};
        commandBuffer->bindVertexBuffers(0, {&binding, 1});
        commandBuffer->draw(3, 1);
        commandBuffer->endRendering();
    });

    (void)app.queue->waitIdle();
    device->destroy(vertexBuffer);
    device->destroy(pipeline);
    return result;
}
