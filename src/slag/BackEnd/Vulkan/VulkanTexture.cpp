#include "VulkanTexture.h"
#include "VulkanLib.h"

namespace slag
{
    namespace vulkan
    {

        VulkanTexture::VulkanTexture(void* texelData, VkDeviceSize dataSize, VkFormat dataFormat, VkFormat textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, VkImageUsageFlags usage,
                                     VkImageLayout initializedLayout, bool destroyImmediately): resources::Resource(destroyImmediately)
        {
            //every texture should support copy
            _usage = usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            _baseFormat = textureFormat;

            VkImageCreateInfo dimg_info{};
            dimg_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            dimg_info.format = _baseFormat;
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
            info.format = _baseFormat;
            info.subresourceRange.layerCount = 1;
            info.subresourceRange.baseMipLevel = 0;
            info.subresourceRange.levelCount = _mipLevels;
            info.subresourceRange.baseArrayLayer = 0;
            info.subresourceRange.aspectMask = usage;

            auto success = vkCreateImageView(VulkanLib::card()->device(),&info, nullptr,&_view);

            auto img = _image;
            auto allocoation = _allocation;
            auto view = _view;

            _disposeFunction = [=]()
            {
                vmaDestroyImage(VulkanLib::card()->memoryAllocator(),img,allocoation);
                vkDestroyImageView(VulkanLib::card()->device(),view, nullptr);
            };

            if(texelData && dataSize)
            {
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

                if(dataFormat != textureFormat)
                {
                    //TODO must implement this...
                    int i=0;
                }

                //clean up temporary resources
                vmaDestroyBuffer(VulkanLib::card()->memoryAllocator(),buffer,tempAllocation);

            }

        }
    }
} // slag