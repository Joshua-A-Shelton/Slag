#include "VulkanTexture.h"
#include "VulkanLib.h"
namespace slag
{
    namespace vulkan
    {
        VulkanTexture::VulkanTexture(VkImage image, VkImageView view, VkFormat format, VkImageAspectFlags usage, uint32_t width, uint32_t height)
        {

        }

        VulkanTexture::~VulkanTexture()
        {
            //we're a swapchain image, so all other resources are managed at the swapchain level except the view
            if(!_allocation)
            {
                vkDestroyImageView(VulkanLib::graphicsCard()->device(),_view, nullptr);
            }
        }

        PixelFormat VulkanTexture::format()
        {
            return formatFromNative(_baseFormat);
        }

        uint32_t VulkanTexture::mipLevels()
        {
            return _mipLevels;
        }

        uint32_t VulkanTexture::width()
        {
            return _width;
        }

        uint32_t VulkanTexture::height()
        {
            return _height;
        }

        Texture::Usage VulkanTexture::usage()
        {
            if(_usage & VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT)
            {
                return Texture::COLOR;
            }
            else
            {
                if(_usage & VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT && _usage & VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT)
                {
                    return Texture::DEPTH_STENCIL;
                }
                else if(_usage & VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT)
                {
                    return Texture::DEPTH;
                }
                return Texture::STENCIL;
            }
        }

        PixelFormat VulkanTexture::formatFromNative(VkFormat format)
        {
            switch (format)
            {
#define DEFINITION(slagName, texelSize, channelCount, alphaChannel, baseType, mipable, srgb, vulkanName, directXName) case vulkanName: return slagName;
                TEXTURE_FORMAT_DEFINTITIONS(DEFINITION)
#undef DEFINITION
            }
            return PixelFormat::UNDEFINED;
        }

        VkFormat VulkanTexture::formatFromCrossPlatform(PixelFormat format)
        {
            switch (format)
            {
#define DEFINITION(slagName, texelSize, channelCount, alphaChannel, baseType, mipable, srgb, vulkanName, directXName) case slagName: return vulkanName;
                TEXTURE_FORMAT_DEFINTITIONS(DEFINITION)
#undef DEFINITION
            }
            return VK_FORMAT_UNDEFINED;
        }
    } // slag
} // Texture