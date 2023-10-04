#include "VulkanTexture.h"
#include "VulkanLib.h"
namespace slag
{
    namespace vulkan
    {
        VulkanTexture::VulkanTexture(VkImage image, VkImageView view, VkFormat format, VkImageAspectFlags usage, uint32_t width, uint32_t height)
        {
            _image = image;
            _view = view;
            _baseFormat = format;
            _usage = usage;
            _width = width;
            _height = height;
            freeResources = [=]()
            {
                vkDestroyImageView(VulkanLib::graphicsCard()->device(),view, nullptr);
            };
        }

        VulkanTexture::~VulkanTexture()
        {
            destroyDeferred();
        }

        VulkanTexture::VulkanTexture(VulkanTexture&& from)
        {
            move(std::move(from));
        }

        VulkanTexture& VulkanTexture::operator=(VulkanTexture&& from)
        {
            move(std::move(from));
            return *this;
        }

        void VulkanTexture::move(VulkanTexture&& from)
        {
            freeResources = from.freeResources;
            from.freeResources = nullptr;

            std::swap(_baseFormat,from._baseFormat);
            std::swap(_usage,from._usage);
            std::swap(_image, from._image);
            std::swap(_allocation,from._allocation);
            std::swap(_view, from._view);
            std::swap(_width, from._width);
            std::swap(_height, from._height);
            std::swap(_mipLevels, from._mipLevels);
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
#define DEFINITION(slagName, texelSize, channelCount, alphaChannel, baseType, mipable, srgb, vulkanName, directXName) case vulkanName: return PixelFormat::slagName;
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

        VkImageLayout VulkanTexture::layoutFromCrossPlatform(Texture::Layout layout)
        {
            switch (layout)
            {
#define DEFINITION(slagName, vulkanName, directXName) case slagName: return vulkanName;
                TEXTURE_LAYOUT_DEFINTITIONS(DEFINITION)
#undef DEFINITION
            }
            return VK_IMAGE_LAYOUT_UNDEFINED;
        }

        VkImage VulkanTexture::vulkanImage()
        {
            return _image;
        }

        void *VulkanTexture::GPUID()
        {
            return _view;
        }


    } // slag
} // Texture