#include "VulkanTexture.h"

#include "VulkanBackend.h"
#include "VulkanGraphicsCard.h"
#include "slag/exceptions/ResourceCreationError.h"
#include "slag/utilities/SLAG_ASSERT.h"

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
            _type = TextureType::ONE_DIMENSIONAL;

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
            _type = TextureType::TWO_DIMENSIONAL;

            construct(VK_IMAGE_TYPE_2D,formatInfo,0);

        }

        VulkanTexture::VulkanTexture(
            VulkanGraphicsCard* card,
            uint32_t width,
            uint32_t height,
            uint32_t depth,
            PixelFormat format,
            TextureUsageFlags usage,
            uint32_t mipLevels)
        {
            auto formatInfo = card->formatProperties(format);
            if (formatInfo.tiling == TextureTiling::UNSUPPORTED)
            {
                throw ResourceCreationError("Given Pixel Format is not compatible on this graphics card");
            }

            _graphicsCard = card;
            _width = width;
            _height = height;
            _depth = depth;
            _format = format;
            _usage = usage;
            _mipLevels = mipLevels;
            _type = TextureType::THREE_DIMENSIONAL;

            construct(VK_IMAGE_TYPE_3D,formatInfo,0);
        }

        VulkanTexture::VulkanTexture(VulkanGraphicsCard* card, PixelFormat format,
            TextureUsageFlags usage, uint32_t dimension, uint32_t mipLevels, uint32_t arrayDepth)
        {
            SLAG_ASSERT(dimension > 0 && "Texture must have a dimension of at least 1");
            auto formatInfo = card->formatProperties(format);
            if (formatInfo.tiling == TextureTiling::UNSUPPORTED)
            {
                throw ResourceCreationError("Given Pixel Format is not compatible on this graphics card");
            }

            _graphicsCard = card;
            _width = dimension;
            _height = dimension;
            _format = format;
            _usage = usage;
            _mipLevels = mipLevels;
            _layers = arrayDepth*6;
            _type = TextureType::CUBE_MAP;

            construct(VK_IMAGE_TYPE_2D,formatInfo,VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);
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
            SLAG_ASSERT(_width>0 && "Texture must have a width of at least 1");
            SLAG_ASSERT(_height>0 && "Texture must have a height of at least 1");
            SLAG_ASSERT(_depth>0 && "Texture must have a depth of at least 1");
            SLAG_ASSERT(_layers>0 && "Texture must have a layer count of at least 1");
            SLAG_ASSERT(_mipLevels>0 && "Texture must have a mip level count of at least 1");
            SLAG_ASSERT((_usage & (TextureUsageFlags::COLOR_TARGET | TextureUsageFlags::DEPTH_STENCIL_TARGET)) != (TextureUsageFlags::COLOR_TARGET | TextureUsageFlags::DEPTH_STENCIL_TARGET) && "Texture cannot be both a color target and a depth/stencil target");
            SLAG_ASSERT(((_mipLevels > 1 && _sampleCount == SampleCount::ONE) || (_sampleCount != SampleCount::ONE && _mipLevels == 1) || (_mipLevels == 1 && _sampleCount == SampleCount::ONE)) && "Texture cannot have both multiple mip levels and have a sample count greater than one");

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
