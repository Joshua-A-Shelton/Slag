#ifndef SLAG_VULKANTEXTURE_H
#define SLAG_VULKANTEXTURE_H
#include "../../Texture.h"
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"
namespace slag
{
    namespace vulkan
    {
        class VulkanTexture: public Texture
        {
        public:
            VulkanTexture(VkImage image, VkImageView view, VkFormat format, VkImageAspectFlags usage, uint32_t width, uint32_t height);
            VulkanTexture(const VulkanTexture&)=delete;
            VulkanTexture& operator=(const VulkanTexture&)=delete;
            VulkanTexture(VulkanTexture&& from);
            VulkanTexture& operator=(VulkanTexture&& from);
            ~VulkanTexture()override;
            void* GPUID()override;
            PixelFormat format()override;
            uint32_t mipLevels()override;
            uint32_t width()override;
            uint32_t height()override;
            Usage usage()override;
            VkImage vulkanImage();
            static PixelFormat formatFromNative(VkFormat format);
            static VkFormat formatFromCrossPlatform(PixelFormat format);
            static VkImageLayout layoutFromCrossPlatform(Texture::Layout layout);
        private:
            void move(VulkanTexture&& from);
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