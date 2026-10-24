// The render graph module (ADR 0072): a compute pass animates a transient
// texture on the compute queue; a graphics pass draws it full screen into the
// swapchain image, with a triangle on top. The graph works out the semaphore
// between the queues, every barrier, the acquired image's wait, and the move
// to Present.

#include "SampleApp.h"

#include <slag/rendergraph/RenderGraph.h>

#include <cstdio>
#include <cstring>

namespace {

constexpr uint32_t textureSize = 512;
constexpr uint32_t samplerSlot = 0;

struct Vertex
{
    float x, y;
    float r, g, b;
};

void printError(slag::MessageSeverity, slag::MessageSource, const char* message, void* userData)
{
    std::fprintf(stderr, "%s\n", message);
    ++*static_cast<uint32_t*>(userData);
}

} // namespace

int main(int argc, char** argv)
{
    SampleApp app("Slag render graph", parseOptions(argc, argv));
    if (!app.ok())
    {
        return 1;
    }
    slag::GraphicsDevice* device = app.device;

    SampleShader plasma("Plasma", "computeMain");
    SampleShader fullScreenVertex("FullScreenTexture", "vertexMain");
    SampleShader fullScreenPixel("FullScreenTexture", "pixelMain");
    SampleShader triangleVertex("Triangle", "vertexMain");
    SampleShader trianglePixel("Triangle", "pixelMain");
    const slag::PixelFormat colorFormats[] = {app.colorFormat};
    slag::Pipeline* plasmaPipeline =
        device->createComputePipeline({.compute = plasma.stage("computeMain"), .debugName = "plasma"}).value();
    slag::Pipeline* fullScreenPipeline = device
                                             ->createGraphicsPipeline({.vertex = fullScreenVertex.stage("vertexMain"),
                                                                       .pixel = fullScreenPixel.stage("pixelMain"),
                                                                       .colorFormats = colorFormats,
                                                                       .debugName = "full-screen texture"})
                                             .value();
    const slag::VertexAttribute attributes[] = {
        {.location = 0, .format = slag::PixelFormat::R32G32Float, .slot = 0, .offset = 0},
        {.location = 1, .format = slag::PixelFormat::R32G32B32Float, .slot = 0, .offset = 8}};
    const slag::VertexStepRate slots[] = {slag::VertexStepRate::PerVertex};
    slag::Pipeline* trianglePipeline =
        device
            ->createGraphicsPipeline({.vertex = triangleVertex.stage("vertexMain"),
                                      .pixel = trianglePixel.stage("pixelMain"),
                                      .vertexInput = {.attributes = attributes, .slots = slots},
                                      .colorFormats = colorFormats,
                                      .debugName = "triangle"})
            .value();
    const Vertex vertices[] = {
        {0.0f, 0.4f, 1, 0.2f, 0.2f}, {0.4f, -0.35f, 0.2f, 1, 0.2f}, {-0.4f, -0.35f, 0.2f, 0.3f, 1}};
    slag::Buffer* vertexBuffer = device
                                     ->createBuffer({.size = sizeof(vertices),
                                                     .placement = slag::Placement::automatic(slag::MemoryKind::Upload)})
                                     .value();
    uint32_t graphErrors = 0;
    slag::Result<slag::rg::RenderGraph*> created = slag::rg::createRenderGraph(
        {.device = device, .graphicsQueue = app.queue, .messageCallback = printError, .messageUserData = &graphErrors});
    if (!plasmaPipeline || !fullScreenPipeline || !trianglePipeline || !vertexBuffer || !created)
    {
        return 1;
    }
    slag::rg::RenderGraph* graph = created.value();
    std::memcpy(vertexBuffer->mappedData(), vertices, sizeof(vertices));
    device->writeSampler(samplerSlot, {});

    uint64_t frame = 0;
    int result = app.runFrames([&](uint32_t image, uint32_t width, uint32_t height) {
        if (!graph->beginFrame())
        {
            return false;
        }
        slag::rg::TextureRef backbuffer = graph->importTexture(
            app.swapchain->texture(image), {.initialLayout = slag::TextureLayout::Undefined,
                                            .finalLayout = slag::TextureLayout::Present,
                                            .attachmentView = app.swapchain->srgbAttachmentView(image),
                                            .swapchain = app.swapchain});
        slag::rg::TextureRef texture = graph->createTexture({.format = slag::PixelFormat::R8G8B8A8Unorm,
                                                             .width = textureSize,
                                                             .height = textureSize,
                                                             .usage = slag::TextureUsage::ShaderWrite,
                                                             .debugName = "plasma"});
        float time = static_cast<float>(frame++) / 60.0f;

        graph->addPass(
            "plasma", slag::CommandQueueType::Compute,
            [&](slag::rg::PassBuilder& pass) { pass.use(texture, slag::rg::Access::ShaderWrite); },
            [=](slag::rg::PassContext& context, slag::CommandBuffer* commandBuffer) {
                // Descriptor slots per frame in flight: 1 + 2 * slot is the
                // writable view, 2 + 2 * slot the sampled one.
                uint32_t slot = 1 + 2 * context.frameSlot();
                device->writeRWTexture(slot, {.texture = context.texture(texture)});
                commandBuffer->bindPipeline(plasmaPipeline);
                struct
                {
                    uint32_t texture;
                    uint32_t size;
                    float time;
                } push = {slot, textureSize, time};
                commandBuffer->pushConstants(&push, sizeof(push));
                commandBuffer->dispatch(textureSize / 8, textureSize / 8, 1);
            });

        graph->addPass(
            "composite", slag::CommandQueueType::Graphics,
            [&](slag::rg::PassBuilder& pass) {
                pass.renderTarget(backbuffer, slag::LoadOp::Discard);
                pass.use(texture, slag::rg::Access::ShaderRead);
            },
            [=](slag::rg::PassContext& context, slag::CommandBuffer* commandBuffer) {
                uint32_t slot = 2 + 2 * context.frameSlot();
                device->writeTexture(slot, {.texture = context.texture(texture)});
                commandBuffer->bindPipeline(fullScreenPipeline);
                struct
                {
                    uint32_t texture;
                    uint32_t sampler;
                    float aspect;
                } push = {slot, samplerSlot, static_cast<float>(height) / static_cast<float>(width)};
                commandBuffer->pushConstants(&push, sizeof(push));
                commandBuffer->draw(3, 1);
                commandBuffer->bindPipeline(trianglePipeline);
                const slag::VertexBufferBinding binding = {vertexBuffer, 0, sizeof(vertices), sizeof(Vertex)};
                commandBuffer->bindVertexBuffers(0, {&binding, 1});
                commandBuffer->draw(3, 1);
            });

        if (!graph->compile())
        {
            return false;
        }
        for (uint32_t i = 0; i < graph->passCount(); ++i)
        {
            if (!graph->recordPass(i))
            {
                return false;
            }
        }
        return static_cast<bool>(graph->submit());
    });

    slag::rg::destroy(graph);
    device->destroy(vertexBuffer);
    device->destroy(trianglePipeline);
    device->destroy(fullScreenPipeline);
    device->destroy(plasmaPipeline);
    if (graphErrors != 0)
    {
        std::printf("%u render graph errors\n", graphErrors);
        return 1;
    }
    return result;
}
