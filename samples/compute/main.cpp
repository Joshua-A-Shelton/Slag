// A compute shader animates a RWTexture each frame; a full-screen triangle
// draws it. Both run on the graphics queue, with a barrier between them that
// moves the texture from ShaderWrite to ShaderRead and back.

#include "SampleApp.h"

namespace {

constexpr uint32_t textureSize = 512;
constexpr uint32_t rwTextureSlot = 0;
constexpr uint32_t textureSlot = 1;
constexpr uint32_t samplerSlot = 0;

} // namespace

int main(int argc, char** argv)
{
    SampleApp app("Slag compute", parseOptions(argc, argv));
    if (!app.ok())
    {
        return 1;
    }
    slag::GraphicsDevice* device = app.device;

    SampleShader plasma("Plasma", "computeMain");
    SampleShader vertexShader("FullScreenTexture", "vertexMain");
    SampleShader pixelShader("FullScreenTexture", "pixelMain");
    slag::Pipeline* computePipeline =
        device->createComputePipeline({.compute = plasma.stage("computeMain"), .debugName = "plasma"}).value();
    const slag::PixelFormat colorFormats[] = {app.colorFormat};
    slag::Pipeline* drawPipeline = device
                                       ->createGraphicsPipeline({.vertex = vertexShader.stage("vertexMain"),
                                                                 .pixel = pixelShader.stage("pixelMain"),
                                                                 .colorFormats = colorFormats,
                                                                 .debugName = "full-screen texture"})
                                       .value();
    slag::Texture* texture = device
                                 ->createTexture2D({.format = slag::PixelFormat::R8G8B8A8Unorm,
                                                    .width = textureSize,
                                                    .height = textureSize,
                                                    .usage = slag::TextureUsage::ShaderWrite,
                                                    .debugName = "plasma"})
                                 .value();
    if (!computePipeline || !drawPipeline || !texture)
    {
        return 1;
    }
    device->writeRWTexture(rwTextureSlot, {.texture = texture});
    device->writeTexture(textureSlot, {.texture = texture});
    device->writeSampler(samplerSlot, {});

    uint64_t frame = 0;
    int result =
        app.run([&](slag::CommandBuffer* commandBuffer, slag::AttachmentView* target, uint32_t width, uint32_t height) {
            // The previous frame's draw is done with the texture once this
            // barrier's pixel-shading sync passes; its contents are discarded.
            slag::TextureBarrier toWrite = {.syncBefore = slag::BarrierSync::PixelShading,
                                            .syncAfter = slag::BarrierSync::ComputeShading,
                                            .accessBefore = slag::BarrierAccess::NoAccess,
                                            .accessAfter = slag::BarrierAccess::ShaderWrite,
                                            .layoutBefore = slag::TextureLayout::Undefined,
                                            .layoutAfter = slag::TextureLayout::ShaderWrite,
                                            .texture = texture};
            commandBuffer->barrier({}, {}, {&toWrite, 1});
            commandBuffer->bindPipeline(computePipeline);
            struct
            {
                uint32_t texture;
                uint32_t size;
                float time;
            } computePush = {rwTextureSlot, textureSize, static_cast<float>(frame++) / 60.0f};
            commandBuffer->pushConstants(&computePush, sizeof(computePush));
            commandBuffer->dispatch(textureSize / 8, textureSize / 8, 1);
            slag::TextureBarrier toRead = {.syncBefore = slag::BarrierSync::ComputeShading,
                                           .syncAfter = slag::BarrierSync::PixelShading,
                                           .accessBefore = slag::BarrierAccess::ShaderWrite,
                                           .accessAfter = slag::BarrierAccess::ShaderRead,
                                           .layoutBefore = slag::TextureLayout::ShaderWrite,
                                           .layoutAfter = slag::TextureLayout::ShaderRead,
                                           .texture = texture};
            commandBuffer->barrier({}, {}, {&toRead, 1});

            const slag::ColorAttachment color = {.view = target, .loadOp = slag::LoadOp::Discard};
            commandBuffer->beginRendering({.colorAttachments = {&color, 1}});
            commandBuffer->bindPipeline(drawPipeline);
            struct
            {
                uint32_t texture;
                uint32_t sampler;
                float aspect;
            } drawPush = {textureSlot, samplerSlot, static_cast<float>(height) / static_cast<float>(width)};
            commandBuffer->pushConstants(&drawPush, sizeof(drawPush));
            commandBuffer->draw(3, 1);
            commandBuffer->endRendering();
        });

    (void)app.queue->waitIdle();
    device->destroy(texture);
    device->destroy(drawPipeline);
    device->destroy(computePipeline);
    return result;
}
