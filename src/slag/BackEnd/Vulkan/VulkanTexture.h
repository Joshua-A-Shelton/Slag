#ifndef SLAG_VULKANTEXTURE_H
#define SLAG_VULKANTEXTURE_H
#include "../../Texture.h"
#include "../../Semaphore.h"
#include "../../Resources/Resource.h"
#include "vk_mem_alloc.h"
#include "VulkanCommandBuffer.h"
#include "VulkanLib.h"
#include "VulkanGPUMemoryReference.h"
#include <vulkan/vulkan.h>

namespace slag
{
    namespace vulkan
    {
        class VulkanTexture: public Texture, resources::Resource
        {
        public:
            VulkanTexture(VkImage image, bool ownImage, VkImageView view, bool ownView, Pixels::Format format, uint32_t width, uint32_t height, uint32_t mipLevels, VkImageUsageFlags usage, VkImageAspectFlags aspects, bool destroyImmediately);
            VulkanTexture(VkImage image, bool ownImage, Pixels::Format format, uint32_t width, uint32_t height, uint32_t mipLevels, VkImageUsageFlags usage, VkImageAspectFlags aspects, bool destroyImmediately);

            VulkanTexture(void** texelDataArray, size_t texelDataCount, VkDeviceSize dataSize, Pixels::Format dataFormat, Texture::Type type, uint32_t width, uint32_t height, uint32_t mipLevels, VkImageUsageFlags usage, Texture::Layout initializedLayout, bool destroyImmediately);
            VulkanTexture(Pixels::Format dataFormat, Texture::Type type, uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t layers, uint8_t sampleCount, VkImageUsageFlags usage, bool destroyImmediately);

            ~VulkanTexture()override;
            VulkanTexture(const VulkanTexture&)=delete;
            VulkanTexture& operator=(const VulkanTexture&)=delete;
            VulkanTexture(VulkanTexture&& from);
            VulkanTexture& operator=(VulkanTexture&& from);
            Type type()override;
            uint32_t width()override;
            uint32_t height()override;
            uint32_t mipLevels()override;
            uint32_t layers()override;
            uint8_t sampleCount()override;
            Pixels::Format format()override;
            TextureUsage usage()override;
            void moveMemory(VmaAllocation newAllocation, VulkanCommandBuffer* commandBuffer);

            VkImage image();
            VkImageView view();
            VkImageAspectFlags aspectFlags();
            friend class VulkanGraphicsCard;
        private:
            void move(VulkanTexture&& from);
            void construct(Pixels::Format dataFormat, Texture::Type textureType, uint32_t width, uint32_t height,uint32_t layers, uint32_t mipLevels, uint8_t samples, VkImageUsageFlags usage);
            Pixels::Format _format{};
            Texture::Type _type=TEXTURE_2D;
            VkImageUsageFlags _usage=0;
            VkImageAspectFlags _aspects=0;
            VkImage _image = nullptr;
            VmaAllocation _allocation = nullptr;
            VkImageView _view = nullptr;
            uint32_t _width = 0;
            uint32_t _height = 0;
            uint32_t _mipLevels=1;
            uint32_t _layers=1;
            uint8_t _sampleCount=1;
            VulkanGPUMemoryReference _selfReference{.memoryType = VulkanGPUMemoryReference::Texture, .reference={this}};
        };
    }//vulkan
} // slag

#endif //SLAG_VULKANTEXTURE_H
