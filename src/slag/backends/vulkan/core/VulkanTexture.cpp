#include "VulkanTexture.h"

#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanGraphicsCard.h"
#include "VulkanSemaphore.h"
#include "../VulkanBackend.h"
#include "slag/core/GPUBarriers.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace vulkan
    {
#ifndef SLAG_DISCREET_TEXTURE_LAYOUTS
        VulkanTexture::VulkanTexture(Pixels::Format texelFormat, Type type, UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t depth,uint32_t mipLevels, uint32_t layers, Texture::SampleCount sampleCount)
        {
            initialize(texelFormat,type,usageFlags,width,height,depth,mipLevels,layers,sampleCount,VK_IMAGE_LAYOUT_UNDEFINED);
            VulkanCommandBuffer commandBuffer(GPUQueue::QueueType::TRANSFER);
            VulkanSemaphore finished(0);
            commandBuffer.begin();
            commandBuffer.transitionToLayout(this,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_GENERAL,VK_ACCESS_NONE,VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,VK_PIPELINE_STAGE_NONE,VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);
            commandBuffer.end();
            CommandBuffer* cbptr = &commandBuffer;
            SemaphoreValue sv{.semaphore = &finished,.value = 1};
            QueueSubmissionBatch submissionData
            {
                .waitSemaphores = nullptr,
                .waitSemaphoreCount = 0,
                .commandBuffers = &cbptr,
                .commandBufferCount = 1,
                .signalSemaphores = &sv,
                .signalSemaphoreCount = 1,
            };
            VulkanGraphicsCard::selected()->transferQueue()->submit(&submissionData,1);
            finished.waitForValue(1);
        }

        VulkanTexture::VulkanTexture(Pixels::Format texelFormat, Type type, UsageFlags usageFlags, uint32_t width,uint32_t height, uint32_t depth,uint32_t mipLevels, uint32_t layers, Texture::SampleCount sampleCount, void* texelData, uint64_t texelDataLength, TextureBufferMapping* mappings, uint32_t mappingCount)
        {
            SLAG_ASSERT(texelData != nullptr && mappings !=nullptr && mappingCount > 0);
            initialize(texelFormat,type,usageFlags,width,height,depth,mipLevels,layers,sampleCount,VK_IMAGE_LAYOUT_UNDEFINED);
            VulkanCommandBuffer commandBuffer(GPUQueue::QueueType::TRANSFER);
            VulkanSemaphore finished(0);

            VulkanBuffer dataBuffer(texelData,texelDataLength,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::DATA_BUFFER);
            commandBuffer.begin();
            commandBuffer.transitionToLayout(this,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_GENERAL,VK_ACCESS_NONE,VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_NONE,VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);
            commandBuffer.copyBufferToTexture(&dataBuffer,this,mappings,mappingCount);
            commandBuffer.end();

            CommandBuffer* cbptr= &commandBuffer;
            SemaphoreValue sv{.semaphore = &finished,.value = 1};

            QueueSubmissionBatch submissionData
            {
                .waitSemaphores = nullptr,
                .waitSemaphoreCount = 0,
                .commandBuffers = &cbptr,
                .commandBufferCount = 1,
                .signalSemaphores = &sv,
                .signalSemaphoreCount = 1,
            };
            VulkanGraphicsCard::selected()->transferQueue()->submit(&submissionData,1);

            finished.waitForValue(1);
        }
#else
        VulkanTexture::VulkanTexture(Pixels::Format texelFormat, Type type, UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t depth,uint32_t mipLevels, uint32_t layers, Texture::SampleCount sampleCount)
        {
            initialize(texelFormat,type,usageFlags,width,height,layers,depth,mipLevels,sampleCount,VK_IMAGE_LAYOUT_UNDEFINED);
        }

        VulkanTexture::VulkanTexture(Pixels::Format texelFormat, Type type, TextureLayouts::Layout initialLayout, UsageFlags usageFlags, uint32_t width,uint32_t height,uint32_t depth,uint32_t mipLevels, uint32_t layers, Texture::SampleCount sampleCount, void* texelData, uint64_t texelDataLength,uint32_t providedDataMips, uint32_t providedDataLayers)
        {
            throw std::runtime_error("Not Implemented");
        }
#endif


        VulkanTexture::VulkanTexture(VkImage image, VkImageView view, Pixels::Format format, Type type, uint32_t width, uint32_t height, uint32_t depth,uint32_t mipLevels, uint32_t layers, UsageFlags usageFlags, Texture::SampleCount sampleCount)
        {
            _image = image;
            _view = view;
            _format = format;
            _type = type;
            _width = width;
            _height = height;
            _depth = depth;
            _layers = layers;
            _mipLevels = mipLevels;
            _usageFlags = usageFlags;
            _sampleCount = sampleCount;
        }

        VulkanTexture::~VulkanTexture()
        {
            if(_allocation)
            {
                vkDestroyImageView(VulkanGraphicsCard::selected()->device(), _view, nullptr);
                vmaDestroyImage(VulkanGraphicsCard::selected()->allocator(),_image,_allocation);
            }
        }

        VulkanTexture::VulkanTexture(VulkanTexture&& from)
        {
            move(from);
        }

        VulkanTexture& VulkanTexture::operator=(VulkanTexture&& from)
        {
            move(from);
            return *this;
        }

        Texture::Type VulkanTexture::type()
        {
            return _type;
        }

        Texture::UsageFlags VulkanTexture::usageFlags()
        {
            return _usageFlags;
        }

        Texture::SampleCount VulkanTexture::sampleCount()
        {
            return _sampleCount;
        }

        uint32_t VulkanTexture::width()
        {
            return _width;
        }

        uint32_t VulkanTexture::height()
        {
            return _height;
        }

        uint32_t VulkanTexture::depth()
        {
            return _depth;
        }

        uint32_t VulkanTexture::layers()
        {
            return _layers;
        }

        uint32_t VulkanTexture::mipLevels()
        {
            return _mipLevels;
        }

        Pixels::Format VulkanTexture::format()
        {
            return _format;
        }

        VkImage VulkanTexture::vulkanHandle()
        {
            return _image;
        }

        VkImageView VulkanTexture::vulkanViewHandle()
        {
            return _view;
        }

        VulkanImageMoveData VulkanTexture::moveMemory(VmaAllocation tempAllocation,CommandBuffer* transitionToGeneralBuffer, CommandBuffer* copyDataBuffer)
        {

            auto vulkanizedFormat = VulkanBackend::vulkanizedFormat(_format);
            auto pixelProperties = Pixels::formatProperties(_format);

            VkImageCreateInfo imageCreateInfo{};
            imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageCreateInfo.format = vulkanizedFormat.format;
            imageCreateInfo.usage = VulkanBackend::vulkanizedUsage(_usageFlags);

            VkExtent3D imageExtent;
            imageExtent.width = static_cast<uint32_t>(_width);
            imageExtent.height = static_cast<uint32_t>(_height);
            imageExtent.depth = static_cast<uint32_t>(_depth);

            auto imageType = VulkanBackend::vulkanizedImageType(_type);
            imageCreateInfo.extent = imageExtent;
            imageCreateInfo.imageType = imageType;
            if(_type == Texture::Type::TEXTURE_CUBE)
            {
                imageCreateInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            }
            imageCreateInfo.mipLevels = _mipLevels;
            imageCreateInfo.arrayLayers = _layers;
            imageCreateInfo.samples = static_cast<VkSampleCountFlagBits>(_sampleCount);
            if (pixelProperties.tiling == PixelFormatProperties::Tiling::OPTIMIZED)
            {
                imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            }
            else
            {
                imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
            }
            imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            VkImage newImage;

            auto result = vkCreateImage(VulkanGraphicsCard::selected()->device(),&imageCreateInfo,nullptr,&newImage);
            if(result != VK_SUCCESS)
            {
                return VulkanImageMoveData{false,nullptr,nullptr};
            }

            auto aspectFlags = Pixels::aspectFlags(_format);
            VkImageAspectFlags vulkanAspectFlags =0;

            if (static_cast<bool>(aspectFlags & Pixels::AspectFlags::COLOR))
            {
                vulkanAspectFlags |= VK_IMAGE_ASPECT_COLOR_BIT;
            }
            else if (static_cast<bool>(aspectFlags & Pixels::AspectFlags::DEPTH))
            {
                vulkanAspectFlags |= VK_IMAGE_ASPECT_DEPTH_BIT;
                if (static_cast<bool>(aspectFlags & Pixels::AspectFlags::STENCIL))
                {
                    vulkanAspectFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;
                }
            }

            VkImageViewCreateInfo viewCreateInfo = {};
            viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewCreateInfo.flags = 0;
            viewCreateInfo.image = _image;
            viewCreateInfo.format = vulkanizedFormat.format;
            viewCreateInfo.components = vulkanizedFormat.mapping;
            viewCreateInfo.viewType = VulkanBackend::vulkanizedImageViewType(_type,_layers);
            viewCreateInfo.subresourceRange.layerCount = _layers;
            viewCreateInfo.subresourceRange.baseMipLevel = 0;
            viewCreateInfo.subresourceRange.levelCount = _mipLevels;
            viewCreateInfo.subresourceRange.baseArrayLayer = 0;
            viewCreateInfo.subresourceRange.aspectMask = vulkanAspectFlags;

            VkSamplerYcbcrConversionInfo conversion_info{.sType = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO};
            if (_format == Pixels::Format::NV12)
            {
                conversion_info.conversion = NV12_CONVERTER;
                viewCreateInfo.pNext = &conversion_info;
            }
            else if (_format == Pixels::Format::OPAQUE_420)
            {
                conversion_info.conversion = OPAQUE_420_CONVERTER;
                viewCreateInfo.pNext = &conversion_info;
            }
            VkImageView newImageView;
            result = vkCreateImageView(VulkanGraphicsCard::selected()->device(),&viewCreateInfo,nullptr,&newImageView);
            if (result != VK_SUCCESS)
            {
                vkDestroyImage(VulkanGraphicsCard::selected()->device(),newImage, nullptr);
                return VulkanImageMoveData{false,nullptr,nullptr};
            }
            result = vmaBindImageMemory(VulkanGraphicsCard::selected()->allocator(),tempAllocation,newImage);
            if (result != VK_SUCCESS)
            {
                vkDestroyImage(VulkanGraphicsCard::selected()->device(),newImage, nullptr);
                vkDestroyImageView(VulkanGraphicsCard::selected()->device(),newImageView, nullptr);
                return VulkanImageMoveData{false,nullptr,nullptr};
            }

            //transition to general

            auto tcb = static_cast<VulkanCommandBuffer*>(transitionToGeneralBuffer)->vulkanCommandBufferHandle();
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
                .aspectMask = VulkanBackend::vulkanizedAspectFlags(Pixels::aspectFlags(_format)),
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
            auto cdb = static_cast<VulkanCommandBuffer*>(copyDataBuffer)->vulkanCommandBufferHandle();
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

                region.srcOffsets[1].x = Texture::width(i);
                region.srcOffsets[1].y = Texture::height(i);
                region.srcOffsets[1].z = Texture::depth(i);

                region.dstOffsets[1] = region.srcOffsets[1];
            }
            VkBlitImageInfo2 blitImageInfo
            {
                .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
                .srcImage = _image,
                .srcImageLayout = VK_IMAGE_LAYOUT_GENERAL,
                .dstImage = newImage,
                .dstImageLayout = VK_IMAGE_LAYOUT_GENERAL,
                .regionCount = static_cast<uint32_t>(regions.size()),
                .pRegions = regions.data(),
                .filter = VK_FILTER_NEAREST,

            };
            vkCmdBlitImage2(cdb,&blitImageInfo);


            VulkanImageMoveData imageMoveData = {true,_image,_view};
            _image = newImage;
            _view = newImageView;
            return imageMoveData;
        }

        void VulkanTexture::initializeChromaConverters()
        {
            auto nv12Properties = Pixels::formatProperties(Pixels::Format::NV12);
            if (nv12Properties.tiling != PixelFormatProperties::Tiling::UNSUPPORTED)
            {
                auto vulkanFormat = VulkanBackend::vulkanizedFormat(Pixels::Format::NV12);
                VkSamplerYcbcrConversionCreateInfo createInfo{.sType = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO};
                createInfo.format = vulkanFormat.format;
                createInfo.ycbcrModel = VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_709;
                //I believe the difference between narrow and full is HDR... not sure though
                createInfo.ycbcrRange = VK_SAMPLER_YCBCR_RANGE_ITU_NARROW;
                createInfo.components = vulkanFormat.mapping;
                createInfo.xChromaOffset = VK_CHROMA_LOCATION_COSITED_EVEN;
                createInfo.yChromaOffset = VK_CHROMA_LOCATION_COSITED_EVEN;
                createInfo.chromaFilter = VK_FILTER_LINEAR;
                vkCreateSamplerYcbcrConversion(VulkanGraphicsCard::selected()->device(),&createInfo,nullptr,&NV12_CONVERTER);
            }
            auto opaque420Properties = Pixels::formatProperties(Pixels::Format::OPAQUE_420);
            if (opaque420Properties.tiling != PixelFormatProperties::Tiling::UNSUPPORTED)
            {
                auto vulkanFormat = VulkanBackend::vulkanizedFormat(Pixels::Format::OPAQUE_420);
                VkSamplerYcbcrConversionCreateInfo createInfo{.sType = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO};
                createInfo.format = vulkanFormat.format;
                createInfo.ycbcrModel = VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_709;
                //I believe the difference between narrow and full is HDR... not sure though
                createInfo.ycbcrRange = VK_SAMPLER_YCBCR_RANGE_ITU_NARROW;
                createInfo.components = vulkanFormat.mapping;
                createInfo.xChromaOffset = VK_CHROMA_LOCATION_COSITED_EVEN;
                createInfo.yChromaOffset = VK_CHROMA_LOCATION_COSITED_EVEN;
                createInfo.chromaFilter = VK_FILTER_LINEAR;
                vkCreateSamplerYcbcrConversion(VulkanGraphicsCard::selected()->device(),&createInfo,nullptr,&OPAQUE_420_CONVERTER);
            }
        }

        void VulkanTexture::cleanupChromaConverters()
        {
            if (NV12_CONVERTER)
            {
                vkDestroySamplerYcbcrConversion(VulkanGraphicsCard::selected()->device(),NV12_CONVERTER,nullptr);
            }
            if (OPAQUE_420_CONVERTER)
            {
                vkDestroySamplerYcbcrConversion(VulkanGraphicsCard::selected()->device(),OPAQUE_420_CONVERTER,nullptr);
            }
        }

        void VulkanTexture::move(VulkanTexture& from)
        {
            _format = from._format;
            _type = from._type;
            _usageFlags = from._usageFlags;
            _sampleCount = from._sampleCount;
            _width = from._width;
            _height = from._height;
            _depth = from._depth;
            _mipLevels = from._mipLevels;
            _layers = from._layers;
            std::swap(_image,from._image);
            std::swap(_allocation, from._allocation);
            std::swap(_view,from._view);

            if(_allocation)
            {
                vmaSetAllocationUserData(VulkanGraphicsCard::selected()->allocator(),_allocation,&_selfReference);
            }
        }

         void VulkanTexture::initialize(Pixels::Format texelFormat, Type type, UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, uint32_t layers, Texture::SampleCount sampleCount, VkImageLayout initialLayout)
        {
            SLAG_ASSERT(((type != Type::TEXTURE_3D) || (type == Type::TEXTURE_3D && layers == 1)) && "3D textures must only have one layer");
            SLAG_ASSERT(((type == Type::TEXTURE_3D) || (type != Type::TEXTURE_3D && depth ==1)) && "Non 3D textures must only have a depth of 1");
            SLAG_ASSERT(((type !=Type::TEXTURE_1D) || (type == Texture::Type::TEXTURE_1D && height == 1)) && "1D textures must have a height of 1");
            SLAG_ASSERT((type != Type::TEXTURE_CUBE || (type == Texture::Type::TEXTURE_CUBE && layers == 6)) && "Cube Textures must have 6 layers");
            SLAG_ASSERT(width >= 1 && height >= 1 && depth >= 1 && mipLevels >=1 && layers >= 1 && "Width, height, depth, mipLevels and layers must be at least 1");
            _format = texelFormat;
            _type = type;
            _usageFlags = usageFlags;
            _width = width;
            _height = height;
            _depth = depth;
            _layers = layers;
            _mipLevels = mipLevels;
            _sampleCount = sampleCount;

            auto vulkanizedFormat = VulkanBackend::vulkanizedFormat(_format);
            auto pixelProperties = Pixels::formatProperties(texelFormat);
            if (pixelProperties.tiling == PixelFormatProperties::Tiling::UNSUPPORTED)
            {
                throw std::runtime_error("Texel Format unsuitable for creating textures");
            }

            VkImageCreateInfo imageCreateInfo{};
            imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageCreateInfo.format = vulkanizedFormat.format;
            imageCreateInfo.usage = VulkanBackend::vulkanizedUsage(_usageFlags);

            VkExtent3D imageExtent;
            imageExtent.width = static_cast<uint32_t>(_width);
            imageExtent.height = static_cast<uint32_t>(_height);
            imageExtent.depth = static_cast<uint32_t>(depth);

            auto imageType = VulkanBackend::vulkanizedImageType(type);
            imageCreateInfo.extent = imageExtent;
            imageCreateInfo.imageType = imageType;
            if(type == Texture::Type::TEXTURE_CUBE)
            {
                imageCreateInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            }
            imageCreateInfo.mipLevels = _mipLevels;
            imageCreateInfo.arrayLayers = _layers;
            imageCreateInfo.samples = static_cast<VkSampleCountFlagBits>(_sampleCount);
            if (pixelProperties.tiling == PixelFormatProperties::Tiling::OPTIMIZED)
            {
                imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            }
            else
            {
                imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
            }
            imageCreateInfo.initialLayout = initialLayout;

            VmaAllocationCreateInfo allocationCreateInfo = {};
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            //reference this texture to allow for GPU memory defragmentation
            allocationCreateInfo.pUserData = &_selfReference;
            vmaCreateImage(VulkanGraphicsCard::selected()->allocator(),&imageCreateInfo,&allocationCreateInfo,&_image,&_allocation,nullptr);

            auto aspectFlags = Pixels::aspectFlags(_format);
            VkImageAspectFlags vulkanAspectFlags =0;

            if (static_cast<bool>(aspectFlags & Pixels::AspectFlags::COLOR))
            {
                vulkanAspectFlags |= VK_IMAGE_ASPECT_COLOR_BIT;
            }
            else if (static_cast<bool>(aspectFlags & Pixels::AspectFlags::DEPTH))
            {
                vulkanAspectFlags |= VK_IMAGE_ASPECT_DEPTH_BIT;
                if (static_cast<bool>(aspectFlags & Pixels::AspectFlags::STENCIL))
                {
                    vulkanAspectFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;
                }
            }

            VkImageViewCreateInfo viewCreateInfo = {};
            viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewCreateInfo.flags = 0;
            viewCreateInfo.image = _image;
            viewCreateInfo.format = vulkanizedFormat.format;
            viewCreateInfo.components = vulkanizedFormat.mapping;
            viewCreateInfo.viewType = VulkanBackend::vulkanizedImageViewType(type,layers);
            viewCreateInfo.subresourceRange.layerCount = _layers;
            viewCreateInfo.subresourceRange.baseMipLevel = 0;
            viewCreateInfo.subresourceRange.levelCount = _mipLevels;
            viewCreateInfo.subresourceRange.baseArrayLayer = 0;
            viewCreateInfo.subresourceRange.aspectMask = vulkanAspectFlags;

            VkSamplerYcbcrConversionInfo conversion_info{.sType = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO};
            if (texelFormat == Pixels::Format::NV12)
            {
                conversion_info.conversion = NV12_CONVERTER;
                viewCreateInfo.pNext = &conversion_info;
            }
            else if (texelFormat == Pixels::Format::OPAQUE_420)
            {
                conversion_info.conversion = OPAQUE_420_CONVERTER;
                viewCreateInfo.pNext = &conversion_info;
            }
            vkCreateImageView(VulkanGraphicsCard::selected()->device(),&viewCreateInfo,nullptr,&_view);
        }

    } // vulkan
} // slag
