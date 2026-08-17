#include "SlagUtilities.h"
#define STB_IMAGE_IMPLEMENTATION
#include <iostream>
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
            TextureBarrier barrier
            {
                .texture = texture.get(),
                .baseLayer = 0,
                .layerCount = 1,
                .baseMipLevel = 0,
                .mipCount = 1,
                .syncBefore = SyncStages::ALL,
                .syncAfter = SyncStages::ALL,
                .flush = MemoryCaches::COPY_WRITE,
                .invalidate = MemoryCaches::NONE,
                .layoutBefore = TextureLayout::GENERAL,
                .layoutAfter = TextureLayout::GENERAL,
            };
            commandBuffer->insertBarriers(&barrier,1);
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
            graphicsCard->transferQueue()->submit(batch);
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
            BufferBarrier barrier
            {
                .buffer = pixels.get(),
                .offset = 0,
                .length = pixels->size(),
                .syncBefore = SyncStages::ALL,
                .syncAfter = SyncStages::ALL,
                .flush = MemoryCaches::COPY_WRITE,
                .invalidate = MemoryCaches::NONE,
            };
            commandBuffer->insertBarriers(&barrier,1);
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
            card->transferQueue()->submit(batch);
            finished->waitForValue(1);

            auto data = pixels->as<uint8_t>();
            lodepng::encode(path.string().c_str(),data,texture->width(),texture->height());
        }

        ImageSimilarity compareTexture(Texture* texture, uint32_t layer, uint32_t mipLevel,
            std::filesystem::path const& againstPath)
        {
            if (texture->format() != PixelFormat::R8G8B8A8_UNORM)
            {
                throw std::runtime_error("Texture format must be R8G8B8A8_UNORM for comparison");
            }
            auto card = texture->graphicsCard();
            int width, height, channels;
            unique_ptr_custom<stbi_uc> data(stbi_load(againstPath.string().c_str(), &width, &height,&channels,4),[](stbi_uc* uc){stbi_image_free(uc);});

            if (data == nullptr)
            {
                throw std::runtime_error("unable to load file: " + againstPath.string());
            }

            if (texture->mipWidth(mipLevel) != width || texture->mipHeight(mipLevel) != height)
            {
                throw std::runtime_error("Texture size does not match");
            }

            auto commandBuffer = std::unique_ptr<CommandBuffer>(card->newCommandBuffer(QueueType::TRANSFER));
            auto finished = std::unique_ptr<Semaphore>(card->newSemaphore());
            TextureBufferMapping mapping
            {
                   .bufferOffset = 0,
                   .subresource =
                   {
                       .aspect = PixelAspect::COLOR,
                       .mipLevel = 0,
                       .baseArrayLayer = layer,
                       .layerCount = 1,
                  },
                  .offset = {0,0,0},
                  .extent = {(uint32_t)width,(uint32_t)height,1}
            };
            auto pixels = std::unique_ptr<Buffer>(card->newBuffer(width*height*4,BufferCPUAccess::READ_WRITE));
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
            card->transferQueue()->submit(batch);
            finished->waitForValue(1);

            ImageSimilarity similarity{};

            struct pixel
            {
                uint8_t r,g,b,a;
            };

            pixel* compare = pixels->as<pixel>();
            pixel* against = (pixel*)(data.get());
            auto pixelCount = width*height;
            auto totalDifference = 0.0f;
            for (auto i=0; i<pixelCount; i++)
            {
                auto pix1 = compare[i];
                auto pix2 = against[i];
                auto maxRed = std::max(pix1.r,pix2.r)/255.0f;
                auto maxGreen = std::max(pix1.g,pix2.g)/255.0f;
                auto maxBlue = std::max(pix1.b,pix2.b)/255.0f;
                auto maxAlpha = std::max(pix1.a,pix2.a)/255.0f;
                auto minRed = std::min(pix1.r,pix2.r)/255.0f;
                auto minGreen = std::min(pix1.g,pix2.g)/255.0f;
                auto minBlue = std::min(pix1.b,pix2.b)/255.0f;
                auto minAlpha = std::min(pix1.a,pix2.a)/255.0f;

                auto pixDiff = ((maxRed-minRed) + (maxGreen-minGreen) + (maxBlue-minBlue) + (maxAlpha-minAlpha))/4;
                if (pixDiff > similarity.maxPixelDifference)
                {
                    similarity.maxPixelDifference = pixDiff;
                }
                totalDifference += pixDiff;
            }
            similarity.overallSimilarity = 1.0f - totalDifference/pixelCount;

            return similarity;
        }
    }
} // slag