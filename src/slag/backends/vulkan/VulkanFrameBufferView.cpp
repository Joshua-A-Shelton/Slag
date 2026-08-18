#include "VulkanFrameBufferView.h"

#include "VulkanBackend.h"
#include "VulkanGraphicsCard.h"
#include "VulkanTexture.h"
#include "slag/exceptions/ResourceCreationError.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace vulkan
    {
        VulkanFrameBufferView::VulkanFrameBufferView(VulkanGraphicsCard* graphicsCard, Texture* texture, uint32_t mip,
            uint32_t baseLayer, uint32_t layerCount)
        {
            SLAG_ASSERT((bool)(texture->usage() & (TextureUsageFlags::COLOR_TARGET | TextureUsageFlags::DEPTH_STENCIL_TARGET)) && "Texture must have either COLOR_TARGET or DEPTH_STENCIL_TARGET usage flags");
            SLAG_ASSERT(mip < texture->mipLevels() && baseLayer < texture->layers() && baseLayer + layerCount <= texture->layers() && "Cannot assign a texture view into resources beyond the texture bounds");

            auto vulkanTexture = (VulkanTexture*)texture;
            auto descriptorInfo = vulkanTexture->descriptorInfo();
            descriptorInfo.subresourceRange.baseMipLevel = mip;
            descriptorInfo.subresourceRange.levelCount = 1;
            descriptorInfo.subresourceRange.baseArrayLayer = baseLayer;
            descriptorInfo.subresourceRange.layerCount = layerCount;
            if (vkCreateImageView(graphicsCard->device(),&descriptorInfo,nullptr,&_vulkanHandle)!= VK_SUCCESS)
            {
                throw ResourceCreationError("Unable to create a frame buffer view");
            }
            _graphicsCard=graphicsCard;
            _texture=texture;
            _mip=mip;
            _baseLayer=baseLayer;
            _layerCount=layerCount;
        }

        VulkanFrameBufferView::VulkanFrameBufferView(VulkanFrameBufferView&& from) noexcept
        {
            move(from);
        }

        VulkanFrameBufferView& VulkanFrameBufferView::operator=(VulkanFrameBufferView&& from) noexcept
        {
            move(from);
            return *this;
        }

        VulkanFrameBufferView::~VulkanFrameBufferView()
        {
            if (_vulkanHandle!=nullptr)
            {
                vkDestroyImageView(_graphicsCard->device(),_vulkanHandle,nullptr);
            }
        }

        Texture* VulkanFrameBufferView::texture()
        {
            return _texture;
        }

        uint32_t VulkanFrameBufferView::mip()
        {
            return _mip;
        }

        uint32_t VulkanFrameBufferView::baseLayer()
        {
            return _baseLayer;
        }

        uint32_t VulkanFrameBufferView::layerCount()
        {
            return _layerCount;
        }

        GraphicsCard* VulkanFrameBufferView::graphicsCard()
        {
            return _graphicsCard;
        }

        VkImageView VulkanFrameBufferView::vulkanHandle() const
        {
            return _vulkanHandle;
        }

        void VulkanFrameBufferView::move(VulkanFrameBufferView& from)
        {
            std::swap(_graphicsCard,from._graphicsCard);
            std::swap(_vulkanHandle,from._vulkanHandle);
            std::swap(_texture,from._texture);
            std::swap(_mip,from._mip);
            std::swap(_baseLayer,from._baseLayer);
            std::swap(_layerCount,from._layerCount);
        }
    } // vulkan
} // slag