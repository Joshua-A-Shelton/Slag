#ifndef SLAG_VULKANTEXTURE_H
#define SLAG_VULKANTEXTURE_H
#include "../../Texture.h"
#include "../../Resources/Resource.h"
#include "vk_mem_alloc.h"
#include <vulkan/vulkan.h>
namespace slag
{
    namespace vulkan
    {
        class VulkanTexture: public Texture, resources::Resource
        {
        public:
            VulkanTexture(void* texelData, VkDeviceSize dataSize, VkFormat dataFormat, VkFormat textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, VkImageUsageFlags usage, VkImageLayout initializedLayout, bool destroyImmediately);
            ~VulkanTexture();
        private:
            VkFormat _baseFormat = VK_FORMAT_UNDEFINED;
            VkImageAspectFlags _usage=0;
            VkImage _image = nullptr;
            VmaAllocation _allocation = nullptr;
            VkImageView _view = nullptr;
            uint32_t _width = 0;
            uint32_t _height = 0;
            uint32_t _mipLevels=1;
        };
    }//vulkan
} // slag

#endif //SLAG_VULKANTEXTURE_H
