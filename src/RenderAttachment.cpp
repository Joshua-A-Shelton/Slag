#include "RenderAttachment.h"
#include <VkBootstrap.h>
#include <vk_mem_alloc.h>
#include <iostream>

namespace slag
{

    RenderAttachment::RenderAttachment(RenderAttachment::Format format, uint32_t width, uint32_t height, GraphicsCard* card)
    {
        _format = format;
        _width = width;
        _height = height;
        _card = card;
        _attachmentType = AttachmentType::Color;

        VkImageCreateInfo info = { };
        info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.pNext = nullptr;

        info.imageType = VK_IMAGE_TYPE_2D;

        info.format = static_cast<VkFormat>(format);
        info.extent = {width,height,1};

        info.mipLevels = 1;
        info.arrayLayers = 1;
        info.samples = VK_SAMPLE_COUNT_1_BIT;
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        //make attachment a color buffer instead of depth
        info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        VmaAllocationCreateInfo dimg_allocinfo = {};
        dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        dimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkImage image;
        VmaAllocation allocation;

        if(vmaCreateImage(static_cast<VmaAllocator>(card->_vmaAllocator), &info, &dimg_allocinfo, &image, &allocation, nullptr)!=VK_SUCCESS)
        {
            throw std::runtime_error("Unable to create image");
        }
        _vkImage = image;
        _vmaAllocation = allocation;


        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.pNext = nullptr;

        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.image = image;
        //change this for depth buffer
        viewInfo.format = static_cast<VkFormat>(format);
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        //also change this for depth buffer
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        VkImageView view;
        if(vkCreateImageView(static_cast<VkDevice>(card->_vkDevice), &viewInfo, nullptr, &view)!= VK_SUCCESS)
        {
            throw std::runtime_error("Unable to create image view");
        }
        _vkImageView = view;

        VkAttachmentDescription* colorAttachment = new VkAttachmentDescription{};
        colorAttachment->format = static_cast<VkFormat>(format);
        colorAttachment->samples = VK_SAMPLE_COUNT_1_BIT;
        //TODO: I may want to change this so that I don't clear automatically
        colorAttachment->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment->finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        _vkAttachmentDescriptionPointer = colorAttachment;

    }

    ///create attachment from swap chain
    RenderAttachment::RenderAttachment(RenderAttachment::Format format, uint32_t width, uint32_t height, void *vkImage,
                                       void *vkImageView, GraphicsCard *card, AttachmentType attachmentType)
    {
        _format = format;
        _width = width;
        _height = height;
        _card = card;
        _attachmentType = attachmentType;
        _vkImage = vkImage;
        _vkImageView = vkImageView;
        VkAttachmentDescription* attachment = new VkAttachmentDescription{};
        attachment->format = static_cast<VkFormat>(format);
        attachment->samples = VK_SAMPLE_COUNT_1_BIT;
        attachment->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment->finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        _vkAttachmentDescriptionPointer = attachment;
        _isSwapchainManaged = true;
    }

    std::shared_ptr<RenderAttachment> RenderAttachment::CreateAttachmentFromResources(RenderAttachment::Format format, uint32_t width, uint32_t height, void* vkImage, void* vkImageView, GraphicsCard *card, RenderAttachment::AttachmentType attachmentType)
    {
        return std::shared_ptr<RenderAttachment>(new RenderAttachment(format, width,height,vkImage,vkImageView,card,attachmentType));
    }

    RenderAttachment::RenderAttachment(uint32_t width, uint32_t height, GraphicsCard *card)
    {
        _format = static_cast<Format>(VK_FORMAT_D32_SFLOAT);
        _width = width;
        _height = height;
        _card = card;
        _attachmentType = AttachmentType::Depth;

        VkImageCreateInfo info = { };
        info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.pNext = nullptr;

        info.imageType = VK_IMAGE_TYPE_2D;

        info.format = VK_FORMAT_D32_SFLOAT;
        info.extent = {width,height,1};

        info.mipLevels = 1;
        info.arrayLayers = 1;
        info.samples = VK_SAMPLE_COUNT_1_BIT;
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        //make attachment a depth buffer instead of color
        info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        VmaAllocationCreateInfo dimg_allocinfo = {};
        dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        dimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkImage image;
        VmaAllocation allocation;

        if(vmaCreateImage(static_cast<VmaAllocator>(card->_vmaAllocator), &info, &dimg_allocinfo, &image, &allocation, nullptr)!=VK_SUCCESS)
        {
            throw std::runtime_error("Unable to create image");
        }
        _vkImage = image;
        _vmaAllocation = allocation;

        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.pNext = nullptr;

        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.image = image;
        viewInfo.format = VK_FORMAT_D32_SFLOAT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        VkImageView view;
        if(vkCreateImageView(static_cast<VkDevice>(card->_vkDevice), &viewInfo, nullptr, &view)!= VK_SUCCESS)
        {
            throw std::runtime_error("Unable to create image view");
        }
        _vkImageView = view;

        VkAttachmentDescription* depthAttachment = new VkAttachmentDescription {};
        depthAttachment->format = VK_FORMAT_D32_SFLOAT;
        depthAttachment->samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment->storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment->finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        _vkAttachmentDescriptionPointer = depthAttachment;
    }

    uint32_t RenderAttachment::width()
    {
        return _width;
    }

    uint32_t RenderAttachment::height()
    {
        return _height;
    }

    RenderAttachment::Format RenderAttachment::format()
    {
        return _format;
    }

    GraphicsCard *RenderAttachment::allocatedGraphicsCard()
    {
        return _card;
    }

    std::shared_ptr<RenderAttachment> RenderAttachment::CreateAttachment(RenderAttachment::Format format, uint32_t width, uint32_t height, GraphicsCard *card)
    {
        return std::shared_ptr<RenderAttachment>(new RenderAttachment(format,width,height,card));
    }

    std::shared_ptr<RenderAttachment>
    RenderAttachment::CreateDepthAttachment(uint32_t width, uint32_t height, GraphicsCard *card)
    {
        return std::shared_ptr<RenderAttachment>(new RenderAttachment(width,height,card));
    }

    void RenderAttachment::destroy()
    {
        if(!_hasCleanedUp)
        {
            _hasCleanedUp = true;
            if(!_isSwapchainManaged)
            {
                if (_vmaAllocation)
                {
                    vmaDestroyImage(static_cast<VmaAllocator>(_card->_vmaAllocator), static_cast<VkImage>(_vkImage),
                                    static_cast<VmaAllocation>(_vmaAllocation));
                } else
                {
                    vkDestroyImage(static_cast<VkDevice>(_card->_vkDevice),static_cast<VkImage>(_vkImage), nullptr);
                }
            }
            vkDestroyImageView(static_cast<VkDevice>(_card->_vkDevice),static_cast<VkImageView>(_vkImageView), nullptr);
            VkAttachmentDescription* ad_ptr = static_cast<VkAttachmentDescription*>(_vkAttachmentDescriptionPointer);
            delete ad_ptr;
        }
    }

    RenderAttachment::~RenderAttachment()
    {
        destroy();
    }

    RenderAttachment::AttachmentType RenderAttachment::attachmentType()
    {
        return _attachmentType;
    }




}