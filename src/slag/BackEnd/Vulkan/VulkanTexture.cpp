#include "VulkanTexture.h"
#include "VulkanLib.h"
namespace slag
{
    namespace vulkan
    {
        VulkanTexture::VulkanTexture(VkImage image, VkImageView view, VkFormat format, VkImageAspectFlags usage, uint32_t width, uint32_t height, bool destroyImmediate)
        {
            _image = image;
            _view = view;
            _baseFormat = format;
            _usage = usage;
            _width = width;
            _height = height;
            destroyImmediately = destroyImmediate;
            VulkanLib::graphicsCard()->runOneTimeCommands(VulkanLib::graphicsCard()->transferQueue(),VulkanLib::graphicsCard()->transferQueueFamily(),[=](VkCommandBuffer cmdBuffer){
                VkImageMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.image = _image;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                barrier.srcAccessMask = VK_ACCESS_NONE;
                barrier.dstAccessMask = VK_ACCESS_NONE;

                vkCmdPipelineBarrier(cmdBuffer,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                     0, nullptr,
                                     0, nullptr,
                                     1, &barrier);
            });
            freeResources = [=]()
            {
                vkDestroyImageView(VulkanLib::graphicsCard()->device(),view, nullptr);

            };
        }

        VulkanTexture::VulkanTexture(uint32_t width, uint32_t height, uint32_t mipLevels, VkImageAspectFlags usage, Pixels::PixelFormat format, bool destroyImmediate)
        {
            VkDeviceSize bufferSize= Pixels::pixelBytes(format)*width*height;
            std::vector<unsigned char>pixelBuffer(bufferSize);
            create(width,height,mipLevels,usage,format,pixelBuffer.data(),bufferSize,destroyImmediate);
        }

        VulkanTexture::VulkanTexture(uint32_t width, uint32_t height, uint32_t mipLevels, VkImageAspectFlags usage, Pixels::PixelFormat format, void* pixelData, bool destroyImmediate)
        {
            auto size = Pixels::pixelBytes(format)/8;
            VkDeviceSize bufferSize= size*width*height;
            create(width,height,mipLevels,usage,format,pixelData,bufferSize,destroyImmediate);
        }

        VulkanTexture::~VulkanTexture()
        {
            if(_view)
            {
                smartDestroy();
            }
        }

        VulkanTexture::VulkanTexture(VulkanTexture&& from): Resource(std::move(from))
        {
            move(std::move(from));
        }

        VulkanTexture& VulkanTexture::operator=(VulkanTexture&& from)
        {
            Resource::operator=(std::move(from));
            move(std::move(from));
            return *this;
        }

        void VulkanTexture::move(VulkanTexture&& from)
        {

            std::swap(_baseFormat,from._baseFormat);
            std::swap(_usage,from._usage);
            std::swap(_image, from._image);
            std::swap(_allocation,from._allocation);
            std::swap(_view, from._view);
            std::swap(_width, from._width);
            std::swap(_height, from._height);
            std::swap(_mipLevels, from._mipLevels);

        }

        Pixels::PixelFormat VulkanTexture::format()
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

        Pixels::PixelFormat VulkanTexture::formatFromNative(VkFormat format)
        {
            switch (format)
            {
#define DEFINITION(slagName, texelSize, channelCount, alphaChannel, baseType, mipable, srgb, vulkanName, directXName) case vulkanName: return Pixels::PixelFormat::slagName;
                TEXTURE_FORMAT_DEFINTITIONS(DEFINITION)
#undef DEFINITION
            }
            return Pixels::PixelFormat::UNDEFINED;
        }

        VkFormat VulkanTexture::formatFromCrossPlatform(Pixels::PixelFormat format)
        {
            switch (format)
            {
#define DEFINITION(slagName, texelSize, channelCount, alphaChannel, baseType, mipable, srgb, vulkanName, directXName) case Pixels::slagName: return vulkanName;
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

        VkImageAspectFlags VulkanTexture::usageFromCrossPlatform(Texture::Usage usage)
        {
            if(usage & Texture::Usage::COLOR)
            {
                return VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
            }
            else
            {
                if(usage & Texture::STENCIL && usage & Texture::Usage::DEPTH)
                {
                    return VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT | VkImageAspectFlagBits::VK_IMAGE_ASPECT_STENCIL_BIT;
                }
                else if(usage & Texture::Usage::DEPTH)
                {
                    return VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;
                }
                return VkImageAspectFlagBits::VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        }

        uint32_t VulkanTexture::formatSize(VkFormat format)
        {
            switch(format)
            {
#define DEFINITION(slagName, texelSize, channelCount, alphaChannel, baseType, mipable, srgb, vulkanName, directXName) case vulkanName: return texelSize;
                TEXTURE_FORMAT_DEFINTITIONS(DEFINITION)
#undef DEFINITION
            }
            return 0;
        }

        VkImage VulkanTexture::vulkanImage()
        {
            return _image;
        }

        VkImageView VulkanTexture::vulkanView()
        {
            return _view;
        }

        void *VulkanTexture::GPUID()
        {
            return _view;
        }

        VkImageAspectFlags VulkanTexture::usageVulkan()
        {
            return _usage;
        }
        VkFormat VulkanTexture::vulkanFormat()
        {
            return _baseFormat;
        }

        void VulkanTexture::create(uint32_t width, uint32_t height, uint32_t mipLevels, VkImageAspectFlags usage, Pixels::PixelFormat format, void* pixelData, VkDeviceSize bufferSize, bool destroyImmdediate)
        {
            _baseFormat = VulkanTexture::formatFromCrossPlatform(format);
            _width = width;
            _height = height;
            _usage = static_cast<VkImageAspectFlagBits>(usage);
            destroyImmediately = destroyImmdediate;


            VkBufferCreateInfo bufferCreateInfo{};
            bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferCreateInfo.size = bufferSize;
            bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

            VmaAllocationCreateInfo allocationCreateInfo{};
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

            //temporary resources
            VkBuffer buffer;
            VmaAllocation tempAllocation;

            auto success = vmaCreateBuffer(VulkanLib::graphicsCard()->memoryAllocator(),&bufferCreateInfo,&allocationCreateInfo,&buffer,&tempAllocation, nullptr);

            //copy data into temp resources
            void* data;
            vmaMapMemory(VulkanLib::graphicsCard()->memoryAllocator(),tempAllocation,&data);
            memcpy(data,pixelData,static_cast<size_t>(bufferSize));
            vmaUnmapMemory(VulkanLib::graphicsCard()->memoryAllocator(),tempAllocation);

            //create final image
            VkExtent3D imageExtent;
            imageExtent.width = static_cast<uint32_t>(_width);
            imageExtent.height = static_cast<uint32_t>(_height);
            imageExtent.depth = 1;

            VkImageCreateInfo dimg_info{};
            dimg_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            dimg_info.format = _baseFormat;
            VkImageLayout toLayout;
            if(usage & DEPTH_STENCIL)
            {
                if(usage == DEPTH_STENCIL)
                {
                    toLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                }
                else if(usage == DEPTH)
                {
                    toLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
                }
                else
                {
                    toLayout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
                }

                dimg_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            }
            else
            {
                toLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                dimg_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            }

            dimg_info.extent = imageExtent;
            dimg_info.imageType = VK_IMAGE_TYPE_2D;
            dimg_info.mipLevels = mipLevels;
            dimg_info.arrayLayers = 1;
            dimg_info.samples = VK_SAMPLE_COUNT_1_BIT;
            dimg_info.tiling = VK_IMAGE_TILING_OPTIMAL;


            VmaAllocationCreateInfo dimg_allocinfo = {};
            dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

            //allocate and create the image
            vmaCreateImage(VulkanLib::graphicsCard()->memoryAllocator(), &dimg_info, &dimg_allocinfo, &_image, &_allocation, nullptr);

            VulkanLib::graphicsCard()->runOneTimeCommands(VulkanLib::graphicsCard()->transferQueue(),VulkanLib::graphicsCard()->transferQueueFamily(),[=](VkCommandBuffer commandBuffer)
            {
                VkImageSubresourceRange range;
                range.aspectMask = _usage;
                range.baseMipLevel = 0;
                range.levelCount = 1;
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
                vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toTransfer);
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
                vkCmdCopyBufferToImage(commandBuffer, buffer, _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
            });

            //create default image view
            VkImageViewCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            info.pNext = nullptr;

            info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            info.image = _image;
            info.format = _baseFormat;
            info.subresourceRange.layerCount = 1;
            info.subresourceRange.baseMipLevel = 0;
            info.subresourceRange.levelCount = _mipLevels;
            info.subresourceRange.baseArrayLayer = 0;
            info.subresourceRange.aspectMask = usage;

            success = vkCreateImageView(VulkanLib::graphicsCard()->device(),&info, nullptr,&_view);

            //clean up temporary resources
            vmaDestroyBuffer(VulkanLib::graphicsCard()->memoryAllocator(),buffer,tempAllocation);

            auto oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            if(_mipLevels>1)
            {
                updateMipMaps();
                oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            }

            VulkanLib::graphicsCard()->runOneTimeCommands(VulkanLib::graphicsCard()->transferQueue(),VulkanLib::graphicsCard()->transferQueueFamily(),[=](VkCommandBuffer commandBuffer)
            {
                VkImageMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.image = _image;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.subresourceRange.aspectMask = _usage;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = _mipLevels;
                barrier.oldLayout = oldLayout;
                barrier.newLayout = toLayout;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

                vkCmdPipelineBarrier(commandBuffer,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                     0, nullptr,
                                     0, nullptr,
                                     1, &barrier);
            });

            auto img = _image;
            auto allocoation = _allocation;
            auto view = _view;

            freeResources = [=]()
            {
                vmaDestroyImage(VulkanLib::graphicsCard()->memoryAllocator(),img,allocoation);
                vkDestroyImageView(VulkanLib::graphicsCard()->device(),view, nullptr);
            };
        }

        void VulkanTexture::updateMipMaps()
        {
            // Check if image format supports linear blitting
            VkFormatProperties formatProperties;
            vkGetPhysicalDeviceFormatProperties(VulkanLib::graphicsCard()->physicalDevice(), _baseFormat, &formatProperties);

            if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
                throw std::runtime_error("texture image format does not support linear blitting!");
            }

            VulkanLib::graphicsCard()->runOneTimeCommands(VulkanLib::graphicsCard()->transferQueue(),VulkanLib::graphicsCard()->transferQueueFamily(),[=](VkCommandBuffer cmdBuffer)
            {
                VkImageMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.image = _image;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;
                barrier.subresourceRange.levelCount = 1;

                int32_t mipWidth = _width;
                int32_t mipHeight = _height;

                for (uint32_t i = 1; i < _mipLevels; i++)
                {
                    barrier.subresourceRange.baseMipLevel = i - 1;
                    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                    vkCmdPipelineBarrier(cmdBuffer,
                                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                         0, nullptr,
                                         0, nullptr,
                                         1, &barrier);

                    VkImageBlit blit{};
                    blit.srcOffsets[0] = {0, 0, 0};
                    blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
                    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    blit.srcSubresource.mipLevel = i - 1;
                    blit.srcSubresource.baseArrayLayer = 0;
                    blit.srcSubresource.layerCount = 1;
                    blit.dstOffsets[0] = {0, 0, 0};
                    blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
                    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    blit.dstSubresource.mipLevel = i;
                    blit.dstSubresource.baseArrayLayer = 0;
                    blit.dstSubresource.layerCount = 1;

                    vkCmdBlitImage(cmdBuffer,
                                   _image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                   _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   1, &blit,
                                   VK_FILTER_LINEAR);

                    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                    vkCmdPipelineBarrier(cmdBuffer,
                                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                                         0, nullptr,
                                         0, nullptr,
                                         1, &barrier);

                    if (mipWidth > 1) mipWidth /= 2;
                    if (mipHeight > 1) mipHeight /= 2;
                }

                /*barrier.subresourceRange.baseMipLevel = _mipLevels - 1;
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                vkCmdPipelineBarrier(cmdBuffer,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                                     0, nullptr,
                                     0, nullptr,
                                     1, &barrier);*/
            });

        }

        VkClearValue VulkanTexture::clearValueFromCrossPlatform(ClearValue& value)
        {
            VkClearValue result;
            std::memcpy(&result, &value, sizeof(VkClearValue));
            return result;
        }

    } // slag
} // Texture