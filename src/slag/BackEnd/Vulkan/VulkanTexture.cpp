#include "VulkanTexture.h"
#include "VulkanLib.h"
#include "VulkanBuffer.h"

namespace slag
{
    namespace vulkan
    {
        VulkanTexture::VulkanTexture(VkImage image, bool ownImage, VkImageView view, bool ownView, Pixels::Format format, uint32_t width, uint32_t height, uint32_t mipLevels, VkImageUsageFlags usage, VkImageAspectFlags aspects, bool destroyImmediately): resources::Resource(destroyImmediately)
        {
            _image = image;
            _view = view;
            _format = format;
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

        VulkanTexture::VulkanTexture(VkImage image, bool ownImage, Pixels::Format format, uint32_t width, uint32_t height, uint32_t mipLevels, VkImageUsageFlags usage, VkImageAspectFlags aspects, bool destroyImmediately): resources::Resource(destroyImmediately)
        {
            _image = image;
            _format = format;
            _usage = usage;
            _width = width;
            _height = height;
            _mipLevels = mipLevels;

            _aspects = aspects;

            //create default image view
            VkImageViewCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            info.pNext = nullptr;

            auto localFormat = VulkanLib::format(format);

            info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            info.image = _image;
            info.format = localFormat.format;
            info.subresourceRange.layerCount = _layers;
            info.subresourceRange.baseMipLevel = 0;
            info.subresourceRange.levelCount = _mipLevels;
            info.subresourceRange.baseArrayLayer = 0;
            info.subresourceRange.aspectMask = _aspects;
            info.components = localFormat.mapping;

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

        VulkanTexture::VulkanTexture(Pixels::Format dataFormat, Texture::Type type, uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t layers, uint8_t sampleCount, VkImageUsageFlags usage, bool destroyImmediately): resources::Resource(destroyImmediately)
        {
            construct(dataFormat,type,width,height,layers,mipLevels,sampleCount,usage);
        }

        VulkanTexture::VulkanTexture(void** texelDataArray, size_t texelDataCount, VkDeviceSize dataSize, Pixels::Format dataFormat, Texture::Type type, uint32_t width, uint32_t height,
                                     uint32_t mipLevels, VkImageUsageFlags usage, Texture::Layout initializedLayout, bool destroyImmediately): resources::Resource(destroyImmediately)
        {
            construct(dataFormat,type,width,height,texelDataCount,mipLevels,1,usage);
            VulkanCommandBuffer commandBuffer(VulkanLib::card()->transferQueueFamily());
            commandBuffer.begin();

            ImageBarrier imageBarrier
            {
                .texture=this,
                .oldLayout=Texture::Layout::UNDEFINED,
                .newLayout=Texture::Layout::TRANSFER_DESTINATION,
                .accessBefore = BarrierAccessFlags::NONE,
                .accessAfter=BarrierAccessFlags::TRANSFER_WRITE,
                .syncBefore=PipelineStageFlags::NONE,
                .syncAfter =PipelineStageFlags::TRANSFER
            };
            commandBuffer.insertBarriers(&imageBarrier,1, nullptr,0, nullptr,0);
            std::vector<VulkanBuffer> dataBuffers;
            for(int i=0; i<texelDataCount; i++)
            {
                dataBuffers.emplace_back(texelDataArray[i],dataSize,Buffer::Accessibility::CPU_AND_GPU,VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
                commandBuffer.copyBufferToImage(&dataBuffers[i],0,this,Texture::Layout::TRANSFER_DESTINATION,0,0);
            }
            imageBarrier.oldLayout = Texture::Layout::TRANSFER_DESTINATION;
            imageBarrier.newLayout = initializedLayout;
            imageBarrier.syncBefore = PipelineStageFlags::TRANSFER;
            imageBarrier.syncAfter = PipelineStageFlags::ALL_COMMANDS;
            imageBarrier.accessBefore = BarrierAccessFlags::TRANSFER_WRITE;
            imageBarrier.accessAfter = std::bit_cast<BarrierAccess>(VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT);
            commandBuffer.insertBarriers(&imageBarrier,1, nullptr,0, nullptr,0);

            commandBuffer.end();
            VulkanLib::card()->transferQueue()->submit(&commandBuffer);
            commandBuffer.waitUntilFinished();
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
            _format = from._format;
            _type = from._type;
            _aspects = from._aspects;
            _usage = from._usage;
            std::swap(_image,from._image);
            std::swap(_allocation, from._allocation);
            std::swap(_view,from._view);
            _width = from._width;
            _height = from._height;
            _mipLevels = from._mipLevels;
            _layers = from._layers;
            _sampleCount = from._sampleCount;
            if(_allocation)
            {
                vmaSetAllocationUserData(VulkanLib::card()->memoryAllocator(),_allocation,&_selfReference);
            }
        }

        Texture::Type VulkanTexture::type()
        {
            return _type;
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

        uint8_t VulkanTexture::sampleCount()
        {
            return _sampleCount;
        }

        Pixels::Format VulkanTexture::format()
        {
            return _format;
        }

        uint32_t VulkanTexture::layers()
        {
            return _layers;
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

        void VulkanTexture::construct(Pixels::Format dataFormat, Texture::Type textureType, uint32_t width, uint32_t height,uint32_t layers, uint32_t mipLevels, uint8_t samples, VkImageUsageFlags usage)
        {
            //every texture should support copy and compute operations
            _usage = usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            _format = dataFormat;
            _type = textureType;
            _width = width;
            _height = height;
            _layers = layers;
            _mipLevels = mipLevels;
            //must be a power of 2, between 1 and 64!!!!
            _sampleCount = samples;

            VkImageAspectFlags aspectFlags = 0;
            if(_usage & (VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT))
            {
                aspectFlags |= VK_IMAGE_ASPECT_COLOR_BIT;
            }
            if(_usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                assert(dataFormat == Pixels::D32_FLOAT_S8X24_UINT || dataFormat == Pixels::D32_FLOAT || dataFormat == Pixels::D24_UNORM_S8_UINT || dataFormat == Pixels::D16_UNORM);
                if(dataFormat == Pixels::D32_FLOAT_S8X24_UINT || dataFormat == Pixels::D24_UNORM_S8_UINT)
                {
                    aspectFlags |=  VK_IMAGE_ASPECT_STENCIL_BIT;
                }
                aspectFlags |= VK_IMAGE_ASPECT_DEPTH_BIT;
            }
            _aspects = aspectFlags;

            auto localFormat = VulkanLib::format(dataFormat);

            VkImageCreateInfo dimg_info{};
            dimg_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            dimg_info.format = localFormat.format;
            dimg_info.usage = _usage;

            VkExtent3D imageExtent;
            imageExtent.width = static_cast<uint32_t>(_width);
            imageExtent.height = static_cast<uint32_t>(_height);
            imageExtent.depth = 1;


            dimg_info.extent = imageExtent;
            dimg_info.imageType = VulkanLib::imageType(textureType);
            if(textureType == Texture::CUBE_MAP)
            {
                dimg_info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            }
            dimg_info.mipLevels = mipLevels;
            dimg_info.arrayLayers = _layers;
            dimg_info.samples = static_cast<VkSampleCountFlagBits>(_sampleCount);
            dimg_info.tiling = VK_IMAGE_TILING_OPTIMAL;

            VmaAllocationCreateInfo dimg_allocinfo = {};
            dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            //reference this texture to allow for GPU memory defragmentation
            dimg_allocinfo.pUserData = &_selfReference;

            //allocate and create the image
            auto result = vmaCreateImage(VulkanLib::card()->memoryAllocator(), &dimg_info, &dimg_allocinfo, &_image, &_allocation, nullptr);
            if(result!= VK_SUCCESS)
            {
                smartDestroy();
                throw std::runtime_error("unable to create image");
            }

            //create default image view
            VkImageViewCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            info.pNext = nullptr;

            info.viewType = VulkanLib::viewType(textureType,layers);
            info.image = _image;
            info.format = localFormat.format;
            info.subresourceRange.layerCount = 1;
            info.subresourceRange.baseMipLevel = 0;
            info.subresourceRange.levelCount = _mipLevels;
            info.subresourceRange.baseArrayLayer = 0;
            info.subresourceRange.aspectMask = _aspects;
            info.components = localFormat.mapping;

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
        }

        TextureUsage VulkanTexture::usage()
        {
            return std::bit_cast<TextureUsage>(_usage);
        }

    }
} // slag