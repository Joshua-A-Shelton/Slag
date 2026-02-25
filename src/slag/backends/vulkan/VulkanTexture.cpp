#include "VulkanTexture.h"

#include "VulkanBackend.h"
#include "VulkanGraphicsCard.h"
#include "slag/exceptions/ResourceCreationError.h"

namespace slag
{
    namespace vulkan
    {
        VulkanTexture::VulkanTexture(
            VulkanGraphicsCard* card,
            uint32_t width,
            PixelFormat format,
            TextureUsageFlags usage,
            uint32_t mipLevels,
            uint32_t layers)
        {
            auto formatInfo = card->formatProperties(format);
            if (formatInfo.tiling == TextureTiling::UNSUPPORTED)
            {
                throw ResourceCreationError("Given Pixel Format is not compatible on this graphics card");
            }
            _graphicsCard = card;
            _width = width;
            _format = format;
            _usage = usage;
            _mipLevels = mipLevels;
            _layers = layers;

            construct(VK_IMAGE_TYPE_1D,formatInfo,0);

        }

        VulkanTexture::VulkanTexture(
            VulkanGraphicsCard* card,
            uint32_t width,
            uint32_t height,
            PixelFormat format,
            TextureUsageFlags usage,
            uint32_t mipLevels,
            SampleCount sampleCount,
            uint32_t layers)
        {
            auto formatInfo = card->formatProperties(format);
            if (formatInfo.tiling == TextureTiling::UNSUPPORTED)
            {
                throw ResourceCreationError("Given Pixel Format is not compatible on this graphics card");
            }

            _graphicsCard = card;
            _width = width;
            _height = height;
            _format = format;
            _usage = usage;
            _mipLevels = mipLevels;
            _sampleCount = sampleCount;
            _layers = layers;

            construct(VK_IMAGE_TYPE_2D,formatInfo,0);

        }

        VulkanTexture::VulkanTexture(VulkanTexture&& from) noexcept
        {
            move(from);
        }

        VulkanTexture& VulkanTexture::operator=(VulkanTexture&& from) noexcept
        {
            move(from);
            return *this;
        }

        VulkanTexture::~VulkanTexture()
        {
            if (_allocation)
            {
                vmaDestroyImage(_graphicsCard->allocator(),_texture,_allocation);
            }
        }

        uint32_t VulkanTexture::width() const
        {
            return _width;
        }

        uint32_t VulkanTexture::height() const
        {
            return _height;
        }

        uint32_t VulkanTexture::depth() const
        {
            return _depth;
        }

        uint32_t VulkanTexture::layers() const
        {
            return _layers;
        }

        uint32_t VulkanTexture::mipLevels() const
        {
            return _mipLevels;
        }

        PixelFormat VulkanTexture::format() const
        {
            return _format;
        }

        SampleCount VulkanTexture::sampleCount() const
        {
            return _sampleCount;
        }

        TextureType VulkanTexture::type() const
        {
            return _type;
        }

        TextureUsageFlags VulkanTexture::usage() const
        {
            return _usage;
        }

        GraphicsCard* VulkanTexture::graphicsCard()
        {
            return _graphicsCard;
        }

        void* VulkanTexture::userData()
        {
            return _userData;
        }

        void VulkanTexture::setUserData(void* userData)
        {
            _userData = userData;
        }

        void VulkanTexture::move(VulkanTexture& from)
        {
            _descriptorInfo = from._descriptorInfo;
            std::swap(_graphicsCard,from._graphicsCard);
            std::swap(_allocation,from._allocation);
            std::swap(_texture,from._texture);
            std::swap(_userData,from._userData);
            _format = from._format;
            _usage = from._usage;
            _sampleCount = from._sampleCount;
            _type = from._type;
            _width = from._width;
            _height = from._height;
            _depth = from._depth;
            _layers = from._layers;
            _mipLevels = from._mipLevels;

            if (_allocation)
            {
                vmaSetAllocationUserData(_graphicsCard->allocator(),_allocation,&_selfReference);
            }
        }

        void VulkanTexture::construct(VkImageType imageType, PixelFormatProperties pixelProperties, VkImageCreateFlags flags)
        {
            auto vulkanFormat = VulkanBackend::nativeFormat(_format);

            VkImageCreateInfo createInfo = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
            createInfo.format = vulkanFormat.format;
            createInfo.usage = VulkanBackend::nativeTextureUsage(_usage);
            createInfo.extent = {_width,_height,_depth};
            createInfo.mipLevels = _mipLevels;
            createInfo.arrayLayers = _layers;
            createInfo.imageType = imageType;
            createInfo.samples = static_cast<VkSampleCountFlagBits>(_sampleCount);
            createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            createInfo.flags = flags;
            switch (pixelProperties.tiling)
            {
            case TextureTiling::UNSUPPORTED:
                throw ResourceCreationError("Unsupported format");
            case TextureTiling::OPTIMIZED:
                createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                break;
            case TextureTiling::LINEAR:
                createInfo.tiling = VK_IMAGE_TILING_LINEAR;
                break;
            }

            VmaAllocationCreateInfo allocationCreateInfo = {};
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            //reference this texture to allow for GPU memory defragmentation
            allocationCreateInfo.pUserData = &_selfReference;

            vmaCreateImage(_graphicsCard->allocator(),&createInfo,&allocationCreateInfo,&_texture,&_allocation,nullptr);

            _descriptorInfo.image = _texture;
            switch (_type)
            {
            case TextureType::ONE_DIMENSIONAL:
                if (_layers == 1)
                {
                    _descriptorInfo.viewType = VK_IMAGE_VIEW_TYPE_1D;
                }
                else
                {
                    _descriptorInfo.viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
                }
                break;
            case TextureType::TWO_DIMENSIONAL:
                if (_layers == 1)
                {
                    _descriptorInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                }
                else
                {
                    _descriptorInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                }
                break;
            case TextureType::THREE_DIMENSIONAL:
                _descriptorInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
                break;
            case TextureType::CUBE_MAP:
                if (_layers == 1)
                {
                    _descriptorInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
                }
                else
                {
                    _descriptorInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
                }
                break;
            }

            _descriptorInfo.format = vulkanFormat.format;
            _descriptorInfo.components.r = vulkanFormat.rSwizzle;
            _descriptorInfo.components.g = vulkanFormat.gSwizzle;
            _descriptorInfo.components.b = vulkanFormat.bSwizzle;
            _descriptorInfo.components.a = vulkanFormat.aSwizzle;

            _descriptorInfo.subresourceRange =
            {
                .aspectMask = VulkanBackend::nativeTextureAspect(Pixel::aspectFlags(_format)),
                .baseMipLevel = 0,
                .levelCount = _mipLevels,
                .baseArrayLayer = 0,
                .layerCount = _layers
            };
        }
    } // vulkan
} // slag
