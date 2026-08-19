#include "VulkanFrame.h"

#include "VulkanFrameBufferView.h"
#include "VulkanGraphicsCard.h"
#include "VulkanSwapChain.h"

namespace slag
{
    namespace vulkan
    {
        VulkanFrame::VulkanFrame(uint32_t frameIndex, VkImage image, uint32_t width, uint32_t height, PixelFormat format, VulkanSwapChain* parentChain)
        {
            _frameIndex = frameIndex;
            _parentChain = parentChain;
            _renderBuffer = new VulkanTexture(static_cast<VulkanGraphicsCard*>(parentChain->graphicsCard()),TextureType::TWO_DIMENSIONAL,image,format,TextureUsageFlags::COLOR_TARGET,width, height,1,1,1,SampleCount::ONE);
            _frameBufferView = new VulkanFrameBufferView(static_cast<VulkanGraphicsCard*>(parentChain->graphicsCard()),_renderBuffer,0,0,1);
        }

        VulkanFrame::~VulkanFrame()
        {
            if (_renderBuffer)
            {
                delete _renderBuffer;
                delete _frameBufferView;
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

        FrameBufferView* VulkanFrame::defaultView()
        {
            return _frameBufferView;
        }

        void VulkanFrame::move(VulkanFrame& from)
        {
            _frameIndex = from._frameIndex;
            _parentChain = from._parentChain;
            std::swap(_renderBuffer,from._renderBuffer);
            std::swap(_frameBufferView,from._frameBufferView);
        }
    } // vulkan
} // slag