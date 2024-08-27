#include "VulkanTexture.h"
#include "VulkanLib.h"

namespace slag
{
    namespace vulkan
    {

        VulkanTexture::VulkanTexture(void* texelData, VkDeviceSize dataSize, VkFormat dataFormat, VulkanizedFormat textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, VkImageUsageFlags usage,
                                     VkImageLayout initializedLayout, bool generateMips, bool destroyImmediately): resources::Resource(destroyImmediately)
        {
            VulkanCommandBuffer buildBuffer(VulkanLib::card()->computeQueueFamily());
            buildBuffer.begin();
            build(buildBuffer,texelData,dataSize,dataFormat,textureFormat,width,height,mipLevels,usage,initializedLayout,generateMips);
            buildBuffer.end();
            VulkanLib::card()->ComputeQueue()->submit(&buildBuffer);
            buildBuffer.waitUntilFinished();
        }

        VulkanTexture::VulkanTexture(VulkanCommandBuffer* onBuffer, void* texelData, VkDeviceSize dataSize, VkFormat dataFormat, VulkanizedFormat textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, VkImageUsageFlags usage, VkImageLayout initializedLayout, bool generateMips, bool destroyImmediately): resources::Resource(destroyImmediately)
        {
            build(*onBuffer,texelData,dataSize,dataFormat,textureFormat,width,height,mipLevels,usage,initializedLayout,generateMips);
        }

        VulkanTexture::~VulkanTexture()
        {
            if(_allocation)
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
            _usage = from._usage;
            std::swap(_image,from._image);
            std::swap(_allocation, from._allocation);
            std::swap(_view,from._view);
            _width = from._width;
            _height = from._height;
            _mipLevels = from._mipLevels;
        }

        void VulkanTexture::build(VulkanCommandBuffer& onBuffer, void* texelData, VkDeviceSize dataSize, VkFormat dataFormat, VulkanizedFormat textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, VkImageUsageFlags usage, VkImageLayout initializedLayout, bool generateMips)
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

            //allocate and create the image
            vmaCreateImage(VulkanLib::card()->memoryAllocator(), &dimg_info, &dimg_allocinfo, &_image, &_allocation, nullptr);

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
            info.subresourceRange.aspectMask = aspectFlags;
            info.components = _baseFormat.mapping;

            auto success = vkCreateImageView(VulkanLib::card()->device(),&info, nullptr,&_view);

            auto img = _image;
            auto allocation = _allocation;
            auto view = _view;

            _disposeFunction = [=]()
            {
                vmaDestroyImage(VulkanLib::card()->memoryAllocator(),img,allocation);
                vkDestroyImageView(VulkanLib::card()->device(),view, nullptr);
            };

            //copy texel data into buffer
            if(texelData && dataSize)
            {
                /*
                //copy image data
                VkBufferCreateInfo bufferCreateInfo{};
                bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bufferCreateInfo.size = dataSize;
                bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

                VmaAllocationCreateInfo allocationCreateInfo{};
                allocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

                //temporary resources
                VkBuffer buffer;
                VmaAllocation tempAllocation;

                success = vmaCreateBuffer(VulkanLib::card()->memoryAllocator(), &bufferCreateInfo, &allocationCreateInfo, &buffer, &tempAllocation, nullptr);

                //copy data into temp resources
                void* data;
                vmaMapMemory(VulkanLib::card()->memoryAllocator(), tempAllocation, &data);
                memcpy(data, texelData, static_cast<size_t>(dataSize));
                vmaUnmapMemory(VulkanLib::card()->memoryAllocator(), tempAllocation);

                if(dataFormat != textureFormat.format)
                {
                    //TODO must implement this...
                    int i=0;
                }

                //clean up temporary resources
                vmaDestroyBuffer(VulkanLib::card()->memoryAllocator(),buffer,tempAllocation);

                if(generateMips)
                {
                    updateMipMaps(onBuffer);
                }
                */
            }
        }

        void VulkanTexture::updateMipMaps()
        {
            VulkanCommandBuffer commandBuffer(VulkanLib::card()->computeQueueFamily());
            updateMipMaps(commandBuffer);
            VulkanLib::card()->ComputeQueue()->submit(&commandBuffer);
            commandBuffer.waitUntilFinished();
        }

        void VulkanTexture::updateMipMaps(VulkanCommandBuffer& onBuffer)
        {
            switch (onBuffer.commandType())
            {
                case GpuQueue::Graphics:
                    throw std::runtime_error("Generating mip maps on Graphics Queue not implemented yet");
                    break;
                case GpuQueue::Compute:
                    throw std::runtime_error("Generating mip maps on Compute Queue not implemented yet");
                    break;
                case GpuQueue::Transfer:
                    throw std::runtime_error("Cannot generate mip maps on Transfer Queue");
                    break;
            }
        }

        void* VulkanTexture::gpuID()
        {
            return _allocation;
        }
    }
} // slag