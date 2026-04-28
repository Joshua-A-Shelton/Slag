#include "SlagUtilities.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "../third-party/LodePNG/lodepng.h"
template<typename T>
using unique_ptr_custom = std::unique_ptr<T,std::function<void(T*)>>;
namespace slag
{
    namespace utilities
    {
        std::unique_ptr<Texture> loadTexture(std::filesystem::path const& path, GraphicsCard* graphicsCard)
        {
            if (!std::filesystem::exists(path))
            {
                throw std::runtime_error("Texture does not exist");
            }
            int width, height, channels;
            unique_ptr_custom<stbi_uc> data(stbi_load(path.string().c_str(), &width, &height,&channels,4),[](stbi_uc* uc){stbi_image_free(uc);});
            TextureBufferMapping mapping
            {
                .bufferOffset = 0,
                .subresource =
                {
                     .aspect = PixelAspect::COLOR,
                     .mipLevel = 0,
                     .baseArrayLayer = 0,
                     .layerCount = 1,
                },
                .offset = {0,0,0},
                .extent = {(uint32_t)width,(uint32_t)height,1}
            };
            auto texture = std::unique_ptr<Texture>(graphicsCard->newTexture2D((uint32_t)width, (uint32_t)height,PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::SAMPLED,1));
            auto pixels = std::unique_ptr<Buffer>(graphicsCard->newBuffer(width*height*4,BufferCPUAccess::WRITE_ONLY));
            auto pixelsPtr = pixels->as<uint8_t>();
            memcpy(pixelsPtr,data.get(),width*height*4);
            auto commandBuffer = std::unique_ptr<CommandBuffer>(graphicsCard->newCommandBuffer(QueueType::TRANSFER));
            auto finished = std::unique_ptr<Semaphore>(graphicsCard->newSemaphore());

            commandBuffer->begin();
            commandBuffer->copyBufferToTexture(pixels.get(),texture.get(),&mapping,1);
            commandBuffer->end();

            auto cmdBuffer = commandBuffer.get();
            SemaphoreValue signal{.semaphore = finished.get(),.value =1};
            SubmissionBatch batch
            {
                .waitSemaphores = nullptr,
                .waitSemaphoreCount = 0,
                .commandBuffers = &cmdBuffer,
                .commandBufferCount = 1,
                .signalSemaphores = &signal,
                .signalSemaphoreCount = 1,
            };
            graphicsCard->transferQueue()->submit(&batch,1);
            finished->waitForValue(1);
            return texture;
        }

        void saveTexture(std::filesystem::path const& path, Texture* texture)
        {
            auto card = texture->graphicsCard();
            auto pixels = std::unique_ptr<Buffer>(card->newBuffer(texture->bufferSize(PixelAspect::COLOR),BufferCPUAccess::READ_WRITE));
            auto commandBuffer = std::unique_ptr<CommandBuffer>(card->newCommandBuffer(QueueType::TRANSFER));
            auto finished = std::unique_ptr<Semaphore>(card->newSemaphore());
            auto mapping = TextureBufferMapping
            {
                .bufferOffset = 0,
                .subresource =
                {
                     .aspect = PixelAspect::COLOR,
                     .mipLevel = 0,
                     .baseArrayLayer = 0,
                },
                .offset = {0,0,0},
                .extent = {texture->width(),texture->height(),1}
            };
            commandBuffer->begin();
            commandBuffer->copyTextureToBuffer(texture,pixels.get(),&mapping,1);
            commandBuffer->end();
            auto cmdBuffer = commandBuffer.get();
            SemaphoreValue signal{.semaphore = finished.get(),.value =1};
            SubmissionBatch batch
            {
                .waitSemaphores = nullptr,
                .waitSemaphoreCount = 0,
                .commandBuffers = &cmdBuffer,
                .commandBufferCount = 1,
                .signalSemaphores = &signal,
                .signalSemaphoreCount = 1,
            };
            card->transferQueue()->submit(&batch,1);
            finished->waitForValue(1);

            auto data = pixels->as<uint8_t>();
            lodepng::encode(path.string().c_str(),data,texture->width(),texture->height());
        }
    }
} // slag