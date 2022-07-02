#include "FrameBuffer.h"
#include <VkBootstrap.h>

namespace slag
{

    FrameBuffer::FrameBuffer(std::vector<std::shared_ptr<RenderAttachment>>& attachments, uint32_t width, uint32_t height, GraphicsCard* card, std::vector<std::array<float,4>>& clearColors)
    {
        _card = card;
        _attachments = attachments;
        _width = width;
        _height = height;
        std::vector<VkImageView> imageViews(attachments.size());
        std::vector<VkAttachmentDescription> attachmentDescriptions(attachments.size());
        std::vector<VkAttachmentReference> attachmentReferences(attachments.size());
        uint32_t attachmentNumber = 0;
        for(auto& attachment : attachments)
        {
            VkAttachmentReference attachmentReference{};
            attachmentReference.attachment = attachmentNumber;
            if(attachment->attachmentType()== RenderAttachment::AttachmentType::Depth)
            {
                if(attachmentNumber+1==attachments.size())
                {
                    _hasDepthBuffer = true;
                }
                else
                {
                    throw std::runtime_error("Depth buffer must be the last one specified if framebuffer is to have one");
                }
                attachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            }
            else
            {
                attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            }

            imageViews[attachmentNumber] = static_cast<VkImageView>(attachment->_vkImageView);
            auto desc = static_cast<VkAttachmentDescription*>(attachment->_vkAttachmentDescriptionPointer);
            attachmentDescriptions[attachmentNumber]=(*desc);

            attachmentReferences[attachmentNumber]=(attachmentReference);
            attachmentNumber++;
        }


        //renderpass
        uint32_t colorAttachments = _hasDepthBuffer? attachmentReferences.size()-1 : attachmentReferences.size();
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = colorAttachments;
        subpass.pColorAttachments = attachmentReferences.data();
        subpass.pDepthStencilAttachment = _hasDepthBuffer? &attachmentReferences.at(colorAttachments): nullptr;

        VkSubpassDependency dependency = {};

        dependency.dstSubpass = 0;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.srcAccessMask = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
        renderPassInfo.pAttachments = attachmentDescriptions.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        VkRenderPass renderPass;

        if (vkCreateRenderPass(static_cast<VkDevice>(card->_vkDevice), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create render pass!");
        }
        _vkRenderPass = renderPass;
        //end renderpass


        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(imageViews.size());
        framebufferInfo.pAttachments = imageViews.data();
        framebufferInfo.width = _width;
        framebufferInfo.height = _height;
        framebufferInfo.layers = 1;

        VkFramebuffer framebuffer;

        if (vkCreateFramebuffer(static_cast<VkDevice>(_card->_vkDevice),&framebufferInfo,nullptr,&framebuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create framebuffer!");
        }
        _vkFrameBuffer = framebuffer;

        if(clearColors.size()==colorAttachmentCount())
        {
            _clearColors = clearColors;
        }
        else
        {
            throw std::runtime_error("clear color count must match color attachments count");
        }
    }

    std::shared_ptr<RenderAttachment> FrameBuffer::getAttachment(size_t index)
    {
        return _attachments.at(index);
    }

    FrameBuffer::~FrameBuffer()
    {
        destroy();
    }

    void FrameBuffer::destroy()
    {
        if(!_hasCleanedUp)
        {
            _hasCleanedUp = true;
            vkDestroyFramebuffer(static_cast<VkDevice>(_card->_vkDevice),static_cast<VkFramebuffer>(_vkFrameBuffer),nullptr);
            vkDestroyRenderPass(static_cast<VkDevice>(_card->_vkDevice),static_cast<VkRenderPass>(_vkRenderPass),nullptr);
            _attachments.clear();
        }
    }

    uint32_t FrameBuffer::width()
    {
        return _width;
    }

    uint32_t FrameBuffer::height()
    {
        return _height;
    }

    size_t FrameBuffer::attachmentCount()
    {
        return _attachments.size();
    }

    bool FrameBuffer::hasDepthBuffer()
    {
        return _hasDepthBuffer;
    }

    std::shared_ptr<FrameBuffer> FrameBuffer::Create(uint32_t width, uint32_t height, GraphicsCard *card)
    {
        auto color = RenderAttachment::CreateAttachment(RenderAttachment::Format::B8G8R8A8_SRGB,width,height,card);
        auto depth = RenderAttachment::CreateDepthAttachment(width,height,card);
        std::vector<std::shared_ptr<RenderAttachment>> attachments = {color,depth};
        std::vector<std::array<float,4>> clear{{1.0f,1.0f,1.0f,1.0f}};
        return std::shared_ptr<FrameBuffer>(new FrameBuffer(attachments,width,height,card,clear));
    }

    std::shared_ptr<FrameBuffer> FrameBuffer::Create(uint32_t width, uint32_t height, GraphicsCard *card, std::array<float,4> clearColor)
    {
        auto color = RenderAttachment::CreateAttachment(RenderAttachment::Format::B8G8R8A8_SRGB,width,height,card);
        auto depth = RenderAttachment::CreateDepthAttachment(width,height,card);
        std::vector<std::shared_ptr<RenderAttachment>> attachments = {color,depth};
        std::vector<std::array<float,4>> clear{{clearColor[0],clearColor[1],clearColor[2],clearColor[3]}};
        return std::shared_ptr<FrameBuffer>(new FrameBuffer(attachments,width,height,card,clear));
    }

    std::shared_ptr<FrameBuffer> FrameBuffer::Create(std::vector<std::shared_ptr<RenderAttachment>>& attachments, uint32_t width, uint32_t height, GraphicsCard *card)
    {
        auto count = attachments.size();
        if(attachments[count-1]->_attachmentType==RenderAttachment::AttachmentType::Depth)
        {
            count--;
        }
        std::vector<std::array<float,4>> clearColors(count);
        for(auto i=0; i< count; i++)
        {
            for(int j=0; j<4; j++)
            {
                clearColors[i][j] = 1.0f;
            }
        }
        return std::shared_ptr<FrameBuffer>(new FrameBuffer(attachments,width,height,card,clearColors));
    }

    std::shared_ptr<FrameBuffer> FrameBuffer::Create(std::vector<std::shared_ptr<RenderAttachment>> &attachments, uint32_t width, uint32_t height, GraphicsCard *card, std::vector<std::array<float,4>> &clearColors)
    {
        return std::shared_ptr<FrameBuffer>(new FrameBuffer(attachments,width,height,card,clearColors));
    }


    size_t FrameBuffer::colorAttachmentCount()
    {
        return _hasDepthBuffer? _attachments.size()-1 : _attachments.size();;
    }

    void FrameBuffer::setClearColors(float *colors, size_t count)
    {
        if(count > colorAttachmentCount())
        {
            throw std::runtime_error("Must not have more clear colors than color attachments");
        }
        size_t current = 0;
        for(int i=0; i< count; i++)
        {
            for(int j=0; j< 4; j++)
            {
                _clearColors[i][j] = colors[current];
                current++;
            }
        }
    }


}
