#include "VulkanTexture.h"

#include "VulkanBackend.h"
#include "VulkanCommandBuffer.h"
#include "VulkanGraphicsCard.h"
#include "VulkanSemaphore.h"
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

        VulkanTexture::VulkanTexture(VulkanGraphicsCard* card, TextureType type, VkImage image, VkImageView view, PixelFormat format, TextureUsageFlags usage, uint32_t width, uint32_t height, uint32_t depth, uint32_t layers, uint32_t mipLevels, SampleCount sampleCount)
        {
            _graphicsCard = card;
            _type = type;
            _texture = image;
            _view = view;
            _format = format;
            _usage = usage;
            _width = width;
            _height = height;
            _depth = depth;
            _layers = layers;
            _mipLevels = mipLevels;
            _sampleCount = sampleCount;
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
            if (_texture)
            {
                if (_allocation)
                {
                    vmaDestroyImage(_graphicsCard->allocator(),_texture,_allocation);
                }
                if (_view)
                {
                    vkDestroyImageView(_graphicsCard->device(),_view,nullptr);
                }
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

        VulkanImageMoveData VulkanTexture::moveMemory(VmaAllocation tempAllocation,
            CommandBuffer* transitionToGeneralBuffer, CommandBuffer* copyDataBuffer)
        {
            auto vulkanizedFormat = VulkanBackend::nativeFormat(_format);
            auto pixelProperties = _graphicsCard->formatProperties(_format);

            VkImageCreateInfo imageCreateInfo{};
            imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageCreateInfo.format = vulkanizedFormat.format;
            imageCreateInfo.usage = VulkanBackend::nativeTextureUsage(_usage);

            VkExtent3D imageExtent;
            imageExtent.width = static_cast<uint32_t>(_width);
            imageExtent.height = static_cast<uint32_t>(_height);
            imageExtent.depth = static_cast<uint32_t>(_depth);


            auto imageType = VK_IMAGE_TYPE_2D;
            switch (_type)
            {
            case TextureType::ONE_DIMENSIONAL:
                imageType = VK_IMAGE_TYPE_1D;
                break;
            case TextureType::TWO_DIMENSIONAL:
                imageType = VK_IMAGE_TYPE_2D;
                break;
            case TextureType::THREE_DIMENSIONAL:
                imageType = VK_IMAGE_TYPE_3D;
                break;
                case TextureType::CUBE_MAP:
                imageType = VK_IMAGE_TYPE_2D;
                imageCreateInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
                break;
            }

            imageCreateInfo.extent = imageExtent;
            imageCreateInfo.imageType = imageType;
            imageCreateInfo.mipLevels = _mipLevels;
            imageCreateInfo.arrayLayers = _layers;
            imageCreateInfo.samples = static_cast<VkSampleCountFlagBits>(_sampleCount);
            if (pixelProperties.tiling == TextureTiling::OPTIMIZED)
            {
                imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            }
            else
            {
                imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
            }
            imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            VkImage newImage;

            auto result = vkCreateImage(_graphicsCard->device(),&imageCreateInfo,nullptr,&newImage);
            if(result != VK_SUCCESS)
            {
                return VulkanImageMoveData{false,nullptr};
            }


            result = vmaBindImageMemory(_graphicsCard->allocator(),tempAllocation,newImage);
            if (result != VK_SUCCESS)
            {
                vkDestroyImage(_graphicsCard->device(),newImage, nullptr);
                return VulkanImageMoveData{false,nullptr};
            }

            //transition to general

            auto tcb = static_cast<VulkanCommandBuffer*>(transitionToGeneralBuffer)->vulkanHandle();
            auto vulkanAspectFlags = VulkanBackend::nativeTextureAspect(Pixel::aspectFlags(_format));
            VkImageMemoryBarrier2 barrier{};

            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            barrier.pNext = nullptr,
            barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE,
            barrier.srcAccessMask = VK_ACCESS_2_NONE,
            barrier.dstStageMask = VK_PIPELINE_STAGE_2_NONE,
            barrier.dstAccessMask = VK_ACCESS_2_NONE,
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL,
            barrier.image = newImage,
            barrier.subresourceRange = VkImageSubresourceRange
            {
                .aspectMask = vulkanAspectFlags,
                .baseMipLevel = 0,
                .levelCount = _mipLevels,
                .baseArrayLayer = 0,
                .layerCount = _layers,
            };


            VkDependencyInfo dependencyInfo{};
            dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            dependencyInfo.memoryBarrierCount = 0;
            dependencyInfo.pMemoryBarriers = nullptr;
            dependencyInfo.bufferMemoryBarrierCount = 0;
            dependencyInfo.pBufferMemoryBarriers = nullptr;
            dependencyInfo.imageMemoryBarrierCount = 1;
            dependencyInfo.pImageMemoryBarriers = &barrier;
            vkCmdPipelineBarrier2(tcb,&dependencyInfo);

            //copy data
            auto cdb = static_cast<VulkanCommandBuffer*>(copyDataBuffer)->vulkanHandle();
            std::vector<VkImageBlit2> regions(_mipLevels,
                {.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
                .srcSubresource = {},
                .srcOffsets = {{0,0,0},{0,0,0}},
                .dstSubresource = {},
                .dstOffsets = {{0,0,0},{0,0,0}}
                });
            for (auto i = 0u; i < _mipLevels; i++)
            {
                auto& region = regions[i];

                region.srcSubresource.baseArrayLayer = 0;
                region.srcSubresource.layerCount = _layers;
                region.srcSubresource.mipLevel = i;
                region.srcSubresource.aspectMask = vulkanAspectFlags;

                region.dstSubresource = region.srcSubresource;

                region.srcOffsets[1].x = Texture::mipWidth(i);
                region.srcOffsets[1].y = Texture::mipHeight(i);
                region.srcOffsets[1].z = Texture::mipDepth(i);

                region.dstOffsets[1] = region.srcOffsets[1];
            }
            VkBlitImageInfo2 blitImageInfo
            {
                .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
                .srcImage = _texture,
                .srcImageLayout = VK_IMAGE_LAYOUT_GENERAL,
                .dstImage = newImage,
                .dstImageLayout = VK_IMAGE_LAYOUT_GENERAL,
                .regionCount = static_cast<uint32_t>(regions.size()),
                .pRegions = regions.data(),
                .filter = VK_FILTER_NEAREST,

            };
            vkCmdBlitImage2(cdb,&blitImageInfo);


            VulkanImageMoveData imageMoveData = {true,_texture};
            _texture = newImage;
            if (_view != nullptr)
            {
                vkDestroyImageView(_graphicsCard->device(),_view,nullptr);
                auto format = VulkanBackend::nativeFormat(_format);
                VkImageViewCreateInfo viewCreateInfo
                {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .image = _texture,
                    .viewType = _descriptorInfo.viewType,
                    .format = format.format,
                    .components ={format.rSwizzle,format.gSwizzle,format.bSwizzle, format.aSwizzle},
                    .subresourceRange =
                        {
                            .aspectMask = VulkanBackend::nativeTextureAspect(Pixel::aspectFlags(_format)),
                            .baseMipLevel = 0,
                            .levelCount = _mipLevels,
                            .baseArrayLayer = 0,
                            .layerCount = _layers,
                        }
                };
                vkCreateImageView(_graphicsCard->device(),&viewCreateInfo,nullptr,&_view);
            }
            return imageMoveData;
        }

        VkImage VulkanTexture::vulkanHandle() const
        {
            return _texture;
        }

        const VkImageViewCreateInfo& VulkanTexture::descriptorInfo()
        {
            return _descriptorInfo;
        }

        VkImageView VulkanTexture::vulkanView() const
        {
            return _view;
        }

        void VulkanTexture::move(VulkanTexture& from)
        {
            _descriptorInfo = from._descriptorInfo;
            std::swap(_graphicsCard,from._graphicsCard);
            std::swap(_allocation,from._allocation);
            std::swap(_texture,from._texture);
            std::swap(_view,from._view);
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

            if (static_cast<bool>(_usage & (TextureUsageFlags::COLOR_TARGET | TextureUsageFlags::DEPTH_STENCIL_TARGET)))
            {

                vkCreateImageView(_graphicsCard->device(),&_descriptorInfo,nullptr,&_view);
            }

            //Transition to general

            VulkanCommandBuffer transitionBuffer(_graphicsCard,QueueType::TRANSFER);
            transitionBuffer.begin();
            auto vkcmdbuffer = transitionBuffer.vulkanHandle();

            VkImageMemoryBarrier2 barrier = {};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            barrier.image = _texture;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            barrier.subresourceRange = _descriptorInfo.subresourceRange;

            VkDependencyInfo dependencyInfo = {};
            dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            dependencyInfo.imageMemoryBarrierCount = 1;
            dependencyInfo.pImageMemoryBarriers = &barrier;
            vkCmdPipelineBarrier2(vkcmdbuffer,&dependencyInfo);
            transitionBuffer.end();
            VulkanSemaphore finished(_graphicsCard,0);
            SemaphoreValue signal{.semaphore = &finished, .value = 1};
            CommandBuffer* commandBuffers = &transitionBuffer;
            SubmissionBatch batch
            {
                .waitSemaphores = nullptr,
                .waitSemaphoreCount = 0,
                .commandBuffers = &commandBuffers,
                .commandBufferCount = 1,
                .signalSemaphores = &signal,
                .signalSemaphoreCount = 1,
            };
            _graphicsCard->transferQueue()->submit(&batch,1);
            finished.waitForValue(1);

        }
    } // vulkan
} // slag
