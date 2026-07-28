#include "VulkanFrame.h"

#include "VulkanGraphicsCard.h"
#include "VulkanSwapChain.h"

namespace slag
{
    namespace vulkan
    {
        VulkanFrame::VulkanFrame(uint32_t frameIndex, VkImage image, VkImageView view, uint32_t width, uint32_t height, PixelFormat format, VulkanSwapChain* parentChain)
        {
            _frameIndex = frameIndex;
            _parentChain = parentChain;
            _renderBuffer = new VulkanTexture(static_cast<VulkanGraphicsCard*>(parentChain->graphicsCard()),TextureType::TWO_DIMENSIONAL,image, view,format,TextureUsageFlags::COLOR_TARGET,width, height,1,1,1,SampleCount::ONE);
        }

        VulkanFrame::~VulkanFrame()
        {
            if (_renderBuffer)
            {
                delete _renderBuffer;
            }
        }

        VulkanFrame::VulkanFrame(VulkanFrame&& from) noexcept
        {
            move(from);
        }

        VulkanFrame& VulkanFrame::operator=(VulkanFrame&& from) noexcept
        {
            move(from);
            return *this;
        }

        Texture* VulkanFrame::renderBuffer()
        {
            return _renderBuffer;
        }

        void VulkanFrame::move(VulkanFrame& from)
        {
            _frameIndex = from._frameIndex;
            _parentChain = from._parentChain;
            std::swap(_renderBuffer,from._renderBuffer);
        }
    } // vulkan
} // slag