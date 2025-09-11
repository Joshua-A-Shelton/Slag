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
        VulkanTexture::VulkanTexture(Pixels::Format texelFormat, Type type, UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t layers, uint32_t mipLevels, Texture::SampleCount sampleCount)
        {
            initialize(texelFormat,type,usageFlags,width,height,layers,mipLevels,sampleCount,VK_IMAGE_LAYOUT_UNDEFINED);
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

        VulkanTexture::VulkanTexture(Pixels::Format texelFormat, Type type, UsageFlags usageFlags, uint32_t width,uint32_t height, uint32_t layers, uint32_t mipLevels, Texture::SampleCount sampleCount, void* texelData,uint32_t providedDataMips, uint32_t providedDataLayers)
        {
            SLAG_ASSERT(texelData != nullptr && providedDataLayers > 0 && providedDataMips > 0);
            initialize(texelFormat,type,usageFlags,width,height,layers,mipLevels,sampleCount,VK_IMAGE_LAYOUT_UNDEFINED);
            VulkanCommandBuffer commandBuffer(GPUQueue::QueueType::TRANSFER);
            VulkanSemaphore finished(0);
            uint64_t bufferSize = 0;
            for (auto i=0; i<mipLevels; i++)
            {
                bufferSize += byteSize(i);
            }
            bufferSize*=layers;
            VulkanBuffer dataBuffer(texelData,bufferSize,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::DATA_BUFFER);
            commandBuffer.begin();
            commandBuffer.transitionToLayout(this,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_GENERAL,VK_ACCESS_NONE,VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_NONE,VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);

            uint64_t offset = 0;
            auto aspectFlags = Pixels::aspectFlags(texelFormat);
            for (uint32_t layer = 0; layer < providedDataLayers; layer++)
            {
                for (uint32_t mip = 0; mip < providedDataMips; mip++)
                {
                    TextureSubresource subresource
                    {
                        .aspectFlags = aspectFlags,
                        .mipLevel = mip,
                        .baseArrayLayer = layer,
                        .layerCount = 1
                    };
                    commandBuffer.copyBufferToTexture(&dataBuffer,offset,this,subresource);
                    offset+=byteSize(mip);
                }
            }

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
        VulkanTexture::VulkanTexture(Pixels::Format texelFormat, Type type, UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t layers, uint32_t mipLevels, Texture::SampleCount sampleCount)
        {
            initialize(texelFormat,type,usageFlags,width,height,layers,mipLevels,sampleCount,VK_IMAGE_LAYOUT_UNDEFINED);
        }

        VulkanTexture::VulkanTexture(Pixels::Format texelFormat, Type type, TextureLayouts::Layout initialLayout, UsageFlags usageFlags, uint32_t width,uint32_t height, uint32_t layers, uint32_t mipLevels, Texture::SampleCount sampleCount, void* texelData,uint32_t providedDataMips, uint32_t providedDataLayers)
        {
            SLAG_ASSERT(texelData != nullptr && providedDataLayers > 0 && providedDataMips > 0);
            initialize(texelFormat,type,usageFlags,width,height,layers,mipLevels,sampleCount,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            VulkanCommandBuffer commandBuffer(GPUQueue::QueueType::TRANSFER);
            VulkanSemaphore finished(0);
            uint64_t bufferSize = 0;
            for (auto i=0; i<mipLevels; i++)
            {
                bufferSize += byteSize(i);
            }
            bufferSize*=layers;
            VulkanBuffer dataBuffer(texelData,bufferSize,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::DATA_BUFFER);
            commandBuffer.begin();
            uint64_t offset = 0;
            auto aspectFlags = Pixels::aspectFlags(texelFormat);
            for (uint32_t layer = 0; layer < providedDataLayers; layer++)
            {
                for (uint32_t mip = 0; mip < providedDataMips; mip++)
                {
                    TextureSubresource subresource
                    {
                        .aspectFlags = aspectFlags,
                        .mipLevel = mip,
                        .baseArrayLayer = layer,
                        .layerCount = 1
                    };
                    commandBuffer.copyBufferToTexture(&dataBuffer,offset,this,subresource,TextureLayouts::Layout::TRANSFER_DESTINATION);
                    offset+=byteSize(mip);
                }
            }

            commandBuffer.insertBarrier(TextureBarrierDiscreet
                {
                    .texture = this,
                    .baseLayer = 0,
                    .layerCount = _layers,
                    .baseMipLevel = 0,
                    .mipCount = _mipLevels,
                    .oldLayout = TextureLayouts::Layout::TRANSFER_DESTINATION,
                    .newLayout = initialLayout,
                    .accessBefore = BarrierAccessFlags::TRANSFER_WRITE,
                    .accessAfter = BarrierAccessFlags::COLOR_ATTACHMENT_READ | BarrierAccessFlags::DEPTH_STENCIL_READ | BarrierAccessFlags::SHADER_READ | BarrierAccessFlags::TRANSFER_READ | BarrierAccessFlags::BLIT_READ,
                    .syncBefore = PipelineStageFlags::TRANSFER,
                    .syncAfter = PipelineStageFlags::ALL_COMMANDS
                });

            commandBuffer.end();

            CommandBuffer* cbptr= &commandBuffer;
            SemaphoreValue sv{.semaphore = &finished,.value = 1};
            VulkanGraphicsCard::selected()->transferQueue()->submit(&cbptr,1,nullptr,0,&sv,1);
            finished.waitForValue(1);
        }
#endif


        VulkanTexture::VulkanTexture(VkImage image, VkImageView view, Pixels::Format format, Type type, uint32_t width, uint32_t height, uint32_t layers, uint32_t mipLevels, UsageFlags usageFlags, Texture::SampleCount sampleCount)
        {
            _image = image;
            _view = view;
            _format = format;
            _type = type;
            _width = width;
            _height = height;
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

         void VulkanTexture::initialize(Pixels::Format texelFormat, Type type, UsageFlags usageFlags, uint32_t width,uint32_t height, uint32_t layers, uint32_t mipLevels, Texture::SampleCount sampleCount, VkImageLayout initialLayout)
        {
            _format = texelFormat;
            _type = type;
            _usageFlags = usageFlags;
            _width = width;
            _height = height;
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
            imageExtent.depth = 1;

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
