#include "VulkanTexture.h"
#include "VulkanLib.h"
#include "VulkanBuffer.h"

namespace slag
{
    namespace vulkan
    {
        VulkanTexture::VulkanTexture(VkImage image, bool ownImage, VkImageView view, bool ownView, VulkanizedFormat format, uint32_t width, uint32_t height, uint32_t mipLevels, VkImageUsageFlags usage, VkImageAspectFlags aspects, bool destroyImmediately): resources::Resource(destroyImmediately)
        {
            _image = image;
            _view = view;
            _baseFormat = format;
            _usage = usage;
            _width = width;
            _height = height;
            _mipLevels = mipLevels;

            _aspects = aspects;

            if(ownImage && ownView)
            {
                _disposeFunction = [=]
                {
                    vkDestroyImageView(VulkanLib::card()->device(),view, nullptr);
                    vkDestroyImage(VulkanLib::card()->device(),image, nullptr);
                };
            }
            else if(ownView)
            {
                _disposeFunction = [=]
                {
                    vkDestroyImageView(VulkanLib::card()->device(),view, nullptr);
                };
            }
            else if(ownImage)
            {
                _disposeFunction = [=]
                {
                    vkDestroyImage(VulkanLib::card()->device(),image, nullptr);
                };
            }
        }

        VulkanTexture::VulkanTexture(VkImage image, bool ownImage, VulkanizedFormat format, uint32_t width, uint32_t height, uint32_t mipLevels, VkImageUsageFlags usage, VkImageAspectFlags aspects, bool destroyImmediately): resources::Resource(destroyImmediately)
        {
            _image = image;
            _baseFormat = format;
            _usage = usage;
            _width = width;
            _height = height;
            _mipLevels = mipLevels;

            _aspects = aspects;

            //create default image view
            VkImageViewCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            info.pNext = nullptr;

            info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            info.image = _image;
            info.format = _baseFormat.format;
            info.subresourceRange.layerCount = 1;
            info.subresourceRange.baseMipLevel = 0;
            info.subresourceRange.levelCount = _mipLevels;
            info.subresourceRange.baseArrayLayer = 0;
            info.subresourceRange.aspectMask = _aspects;
            info.components = _baseFormat.mapping;

            auto success = vkCreateImageView(VulkanLib::card()->device(),&info, nullptr,&_view);

            if(success != VK_SUCCESS)
            {
                throw std::runtime_error("unable to create image view");
            }

            auto view = _view;
            if(ownImage)
            {
                _disposeFunction = [=]
                {
                    vkDestroyImageView(VulkanLib::card()->device(),view, nullptr);
                    vkDestroyImage(VulkanLib::card()->device(),image, nullptr);
                };
            }
            else
            {
                _disposeFunction = [=]
                {
                    vkDestroyImageView(VulkanLib::card()->device(),view, nullptr);
                };
            }
        }

        VulkanTexture::VulkanTexture(void* texelData, VkDeviceSize dataSize, VkFormat dataFormat, VulkanizedFormat textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, VkImageUsageFlags usage,
                                     VkImageLayout initializedLayout, bool generateMips, bool destroyImmediately): resources::Resource(destroyImmediately)
        {
            VulkanCommandBuffer buildBuffer(VulkanLib::card()->computeQueueFamily());
            buildBuffer.begin();
            build(&buildBuffer,texelData,dataSize,dataFormat,textureFormat,width,height,mipLevels,usage,initializedLayout,generateMips);
            buildBuffer.end();
            VulkanLib::card()->computeQueue()->submit(&buildBuffer);
            buildBuffer.waitUntilFinished();

        }

        VulkanTexture::VulkanTexture(VulkanCommandBuffer* onBuffer, void* texelData, VkDeviceSize dataSize, VkFormat dataFormat, VulkanizedFormat textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, VkImageUsageFlags usage, VkImageLayout initializedLayout, bool generateMips, bool destroyImmediately): resources::Resource(destroyImmediately)
        {
            build(onBuffer,texelData,dataSize,dataFormat,textureFormat,width,height,mipLevels,usage,initializedLayout,generateMips);
        }

        VulkanTexture::~VulkanTexture()
        {
            if(_image)
            {
                smartDestroy();
            }
        }

        VulkanTexture::VulkanTexture(VulkanTexture&& from): resources::Resource(from._destroyImmediately)
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
            resources::Resource::move(from);
            _baseFormat = from._baseFormat;
            _aspects = from._aspects;
            std::swap(_image,from._image);
            std::swap(_allocation, from._allocation);
            std::swap(_view,from._view);
            _width = from._width;
            _height = from._height;
            _mipLevels = from._mipLevels;
            if(_allocation)
            {
                vmaSetAllocationUserData(VulkanLib::card()->memoryAllocator(),_allocation,&_selfReference);
            }
        }

        void VulkanTexture::build(VulkanCommandBuffer* onBuffer, void* texelData, VkDeviceSize dataSize, VkFormat dataFormat, VulkanizedFormat textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, VkImageUsageFlags usage, VkImageLayout initializedLayout, bool generateMips)
        {
            //every texture should support copy and compute operations
            _usage = usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
            _baseFormat = textureFormat;
            _width = width;
            _height = height;

            VkImageAspectFlags aspectFlags = 0;
            if(_usage & (VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT))
            {
                aspectFlags |= VK_IMAGE_ASPECT_COLOR_BIT;
            }
            if(_usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                //TODO: I think i may need to separate the depth and stencil based on the texture format
                aspectFlags |= VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            }
            _aspects = aspectFlags;

            VkImageCreateInfo dimg_info{};
            dimg_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            dimg_info.format = _baseFormat.format;
            dimg_info.usage = _usage;

            VkExtent3D imageExtent;
            imageExtent.width = static_cast<uint32_t>(_width);
            imageExtent.height = static_cast<uint32_t>(_height);
            imageExtent.depth = 1;

            dimg_info.extent = imageExtent;
            dimg_info.imageType = VK_IMAGE_TYPE_2D;
            dimg_info.mipLevels = mipLevels;
            dimg_info.arrayLayers = 1;
            dimg_info.samples = VK_SAMPLE_COUNT_1_BIT;
            dimg_info.tiling = VK_IMAGE_TILING_OPTIMAL;

            VmaAllocationCreateInfo dimg_allocinfo = {};
            dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            //reference this texture to allow for GPU memory defragmentation
            dimg_allocinfo.pUserData = &_selfReference;

            //allocate and create the image
             auto result = vmaCreateImage(VulkanLib::card()->memoryAllocator(), &dimg_info, &dimg_allocinfo, &_image, &_allocation, nullptr);
             if(result!= VK_SUCCESS)
             {
                 throw std::runtime_error("unable to create image");
             }

            //create default image view
            VkImageViewCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            info.pNext = nullptr;

            info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            info.image = _image;
            info.format = _baseFormat.format;
            info.subresourceRange.layerCount = 1;
            info.subresourceRange.baseMipLevel = 0;
            info.subresourceRange.levelCount = _mipLevels;
            info.subresourceRange.baseArrayLayer = 0;
            info.subresourceRange.aspectMask = _aspects;
            info.components = _baseFormat.mapping;

            result = vkCreateImageView(VulkanLib::card()->device(),&info, nullptr,&_view);

            auto img = _image;
            auto allocation = _allocation;
            auto view = _view;
            _disposeFunction = [=]()
            {
                vmaDestroyImage(VulkanLib::card()->memoryAllocator(),img,allocation);
                vkDestroyImageView(VulkanLib::card()->device(),view, nullptr);
            };
            if(result != VK_SUCCESS)
            {
                smartDestroy();
                throw std::runtime_error("unable to create image view");
            }

            //copy texel data into buffer
            if(texelData && dataSize)
            {
                //TODO: use fully built buffer class for this, so the resource can outlive the command buffer
                VulkanBuffer dataBuffer(texelData,dataSize,slag::Buffer::CPU,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,false);

                auto commandBuffer = onBuffer->underlyingCommandBuffer();

                if(dataFormat != textureFormat.format)
                {
                    //TODO must implement this...
                    throw std::runtime_error("not implemented");
                }
                else
                {
                    VkImageSubresourceRange range;
                    range.aspectMask = _aspects;
                    range.baseMipLevel = 0;
                    range.levelCount = _mipLevels;
                    range.baseArrayLayer = 0;
                    range.layerCount = 1;

                    VkImageMemoryBarrier imageBarrier_toTransfer = {};
                    imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

                    imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    imageBarrier_toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    imageBarrier_toTransfer.image = _image;
                    imageBarrier_toTransfer.subresourceRange = range;

                    imageBarrier_toTransfer.srcAccessMask = 0;
                    imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                    //barrier the image into the transfer-receive layout
                    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toTransfer);
                    VkBufferImageCopy copyRegion = {};
                    copyRegion.bufferOffset = 0;
                    copyRegion.bufferRowLength = 0;
                    copyRegion.bufferImageHeight = 0;

                    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    copyRegion.imageSubresource.mipLevel = 0;
                    copyRegion.imageSubresource.baseArrayLayer = 0;
                    copyRegion.imageSubresource.layerCount = 1;
                    copyRegion.imageExtent = imageExtent;

                    //copy the buffer into the image
                    vkCmdCopyBufferToImage(commandBuffer, dataBuffer.underlyingBuffer(), _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
                }

                if(generateMips && mipLevels > 1)
                {
                    updateMipMaps(onBuffer,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,initializedLayout);
                }
                //transition into starting layout because we didn't in mip maps
                else
                {
                    VkImageSubresourceRange range;
                    range.aspectMask = _aspects;
                    range.baseMipLevel = 0;
                    range.levelCount = _mipLevels;
                    range.baseArrayLayer = 0;
                    range.layerCount = 1;

                    VkImageMemoryBarrier imageBarrier_toTransfer = {};
                    imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

                    imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    imageBarrier_toTransfer.newLayout = initializedLayout;
                    imageBarrier_toTransfer.image = _image;
                    imageBarrier_toTransfer.subresourceRange = range;

                    imageBarrier_toTransfer.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toTransfer);
                }

            }
        }

        void VulkanTexture::updateMipMaps(VkImageLayout startingLayout, VkImageLayout endingLayout)
        {
            VulkanCommandBuffer commandBuffer(VulkanLib::card()->computeQueueFamily());
            commandBuffer.begin();
            updateMipMaps(&commandBuffer,startingLayout,endingLayout);
            commandBuffer.end();
            VulkanLib::card()->computeQueue()->submit(&commandBuffer);
            commandBuffer.waitUntilFinished();
        }

        void VulkanTexture::updateMipMaps(VulkanCommandBuffer* onBuffer,VkImageLayout startingLayout, VkImageLayout endingLayout)
        {
            assert(onBuffer && "buffer cannot be null");
            switch (onBuffer->commandType())
            {
                case GpuQueue::Graphics:
                    smartDestroy();
                    throw std::runtime_error("Generating mip maps on Graphics Queue not implemented yet");
                    break;
                case GpuQueue::Compute:
                    smartDestroy();
                    throw std::runtime_error("Generating mip maps on Compute Queue not implemented yet");
                    break;
                case GpuQueue::Transfer:
                    smartDestroy();
                    throw std::runtime_error("Cannot generate mip maps on Transfer Queue");
                    break;
            }
        }

        void* VulkanTexture::gpuID()
        {
            return _allocation;
        }

        VkImage VulkanTexture::copyVkImage()
        {
            VkImageCreateInfo dimg_info{};
            dimg_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            dimg_info.format = _baseFormat.format;
            dimg_info.usage = _aspects;

            VkExtent3D imageExtent;
            imageExtent.width = static_cast<uint32_t>(_width);
            imageExtent.height = static_cast<uint32_t>(_height);
            imageExtent.depth = 1;

            dimg_info.extent = imageExtent;
            dimg_info.imageType = VK_IMAGE_TYPE_2D;
            dimg_info.mipLevels = _mipLevels;
            dimg_info.arrayLayers = 1;
            dimg_info.samples = VK_SAMPLE_COUNT_1_BIT;
            dimg_info.tiling = VK_IMAGE_TILING_OPTIMAL;

            VkImage copy;
            vkCreateImage(VulkanLib::card()->device(),&dimg_info, nullptr,&copy);
            return copy;
        }

        uint32_t VulkanTexture::width()
        {
            return _width;
        }

        uint32_t VulkanTexture::height()
        {
            return _height;
        }

        uint32_t VulkanTexture::mipLevels()
        {
            return _mipLevels;
        }

        VkImage VulkanTexture::image()
        {
            return _image;
        }

        VkImageView VulkanTexture::view()
        {
            return _view;
        }

        VkImageAspectFlags VulkanTexture::aspectFlags()
        {
            return _aspects;
        }

    }
} // slag