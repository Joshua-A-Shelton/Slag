#ifndef SLAG_VULKANTEXTURE_H
#define SLAG_VULKANTEXTURE_H
#include "../../Texture.h"
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"
#include "../Resource.h"
#include "../../ClearValue.h"

namespace slag
{
    namespace vulkan
    {
        class VulkanTexture: public Texture, Resource
        {
        public:
            VulkanTexture(VkImage image, VkImageView view, VkFormat format, VkImageAspectFlags usage, uint32_t width, uint32_t height, bool destroyImmediate);

            VulkanTexture(uint32_t width, uint32_t height, uint32_t mipLevels,VkImageAspectFlags usage, Pixels::PixelFormat format, Texture::Layout layout, Features features, bool destroyImmediate);

            VulkanTexture(uint32_t width, uint32_t height, uint32_t mipLevels,VkImageAspectFlags usage, Pixels::PixelFormat format, void* pixelData, Texture::Layout layout, Features features, bool destroyImmediate);

            VulkanTexture(const VulkanTexture&)=delete;
            VulkanTexture& operator=(const VulkanTexture&)=delete;
            VulkanTexture(VulkanTexture&& from);
            VulkanTexture& operator=(VulkanTexture&& from);
            ~VulkanTexture()override;
            void* GPUID()override;
            Pixels::PixelFormat format()override;
            uint32_t mipLevels()override;
            uint32_t width()override;
            uint32_t height()override;
            Usage usage()override;
            void blitImmediate(Texture* source,Rectangle sourceArea, Texture::Layout sourceLayout, Rectangle destinationArea, Texture::Layout destinationLayout,TextureSampler::Filter filter = TextureSampler::Filter::NEAREST)override;
            VkImageAspectFlags usageVulkan();
            VkImage vulkanImage();
            VkImageView vulkanView();
            VkFormat vulkanFormat();
            static Pixels::PixelFormat formatFromNative(VkFormat format);
            static VkFormat formatFromCrossPlatform(Pixels::PixelFormat format);
            static VkImageLayout layoutFromCrossPlatform(Texture::Layout layout);
            static VkImageAspectFlags usageFromCrossPlatform(Texture::Usage usage);
            static VkImageUsageFlags featuresFromCrossPlatform(Texture::Features features);
            static uint32_t formatSize(VkFormat format);
            static VkClearValue clearValueFromCrossPlatform(ClearValue& value);
        private:
            void move(VulkanTexture&& from);
            void create(uint32_t width, uint32_t height, uint32_t mipLevels, VkImageAspectFlags usage, Pixels::PixelFormat format, VkImageLayout toLayout, void* pixelData, VkDeviceSize bufferSize, VkImageUsageFlags features, bool destroyImmdediate);

            void updateMipMaps();
            VkFormat _baseFormat = VK_FORMAT_UNDEFINED;
            VkImageAspectFlags _usage=0;
            VkImage _image = nullptr;
            VmaAllocation _allocation = nullptr;
            VkImageView _view = nullptr;
            uint32_t _width = 0;
            uint32_t _height = 0;
            uint32_t _mipLevels=1;

        };
    } // slag
} // Texture
#endif //SLAG_VULKANTEXTURE_H