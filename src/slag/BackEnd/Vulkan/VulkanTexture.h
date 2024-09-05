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
            VulkanTexture(VkImage image, bool ownImage, VkImageView view, bool ownView, VulkanizedFormat format, uint32_t width, uint32_t height, uint32_t mipLevels, VkImageUsageFlags usage, VkImageAspectFlags aspects, bool destroyImmediately);
            VulkanTexture(VkImage image, bool ownImage, VulkanizedFormat format, uint32_t width, uint32_t height, uint32_t mipLevels, VkImageUsageFlags usage, VkImageAspectFlags aspects, bool destroyImmediately);

            VulkanTexture(void* texelData, VkDeviceSize dataSize, VkFormat dataFormat, VulkanizedFormat textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, VkImageUsageFlags usage, VkImageLayout initializedLayout, bool generateMips, bool destroyImmediately);
            VulkanTexture(VulkanCommandBuffer* onBuffer, void* texelData, VkDeviceSize dataSize, VkFormat dataFormat, VulkanizedFormat textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, VkImageUsageFlags usage, VkImageLayout initializedLayout, bool generateMips, bool destroyImmediately);
            ~VulkanTexture()override;
            VulkanTexture(const VulkanTexture&)=delete;
            VulkanTexture& operator=(const VulkanTexture&)=delete;
            VulkanTexture(VulkanTexture&& from);
            VulkanTexture& operator=(VulkanTexture&& from);
            void updateMipMaps(VkImageLayout startingLayout, VkImageLayout endingLayout);
            void updateMipMaps(VulkanCommandBuffer* onBuffer,VkImageLayout startingLayout, VkImageLayout endingLayout);
            void* gpuID()override;
            uint32_t width()override;
            uint32_t height()override;
            uint32_t mipLevels()override;

            VkImage image();
            VkImageView view();
            VkImageAspectFlags aspectFlags();
            friend class VulkanGraphicsCard;
        private:
            void move(VulkanTexture&& from);
            void build(VulkanCommandBuffer* onBuffer, void* texelData, VkDeviceSize dataSize, VkFormat dataFormat, VulkanizedFormat textureFormat, uint32_t  width, uint32_t height, uint32_t mipLevels, VkImageUsageFlags usage, VkImageLayout initializedLayout, bool generateMips);
            VkImage copyVkImage();
            VulkanizedFormat _baseFormat{};
            VkImageUsageFlags _usage;
            VkImageAspectFlags _aspects=0;
            VkImage _image = nullptr;
            VmaAllocation _allocation = nullptr;
            VkImageView _view = nullptr;
            uint32_t _width = 0;
            uint32_t _height = 0;
            uint32_t _mipLevels=1;
            VulkanGPUMemoryReference _selfReference{.memoryType = VulkanGPUMemoryReference::Texture, .reference={this}};
        };
    }//vulkan
} // slag

#endif //SLAG_VULKANTEXTURE_H
