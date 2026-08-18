#ifndef SLAG_VULKANFRAMEBUFFERVIEW_H
#define SLAG_VULKANFRAMEBUFFERVIEW_H
#include <slag/Slag.h>
#include <vulkan/vulkan.h>
namespace slag
{
    namespace vulkan
    {
        class VulkanGraphicsCard;

        class VulkanFrameBufferView: public FrameBufferView
        {
        public:
            VulkanFrameBufferView(VulkanGraphicsCard* graphicsCard, Texture* texture, uint32_t mip, uint32_t baseLayer, uint32_t layerCount);
            VulkanFrameBufferView(const VulkanFrameBufferView&)=delete;
            VulkanFrameBufferView& operator=(const VulkanFrameBufferView&)=delete;
            VulkanFrameBufferView(VulkanFrameBufferView&& from) noexcept;
            VulkanFrameBufferView& operator=(VulkanFrameBufferView&& from) noexcept;
            ~VulkanFrameBufferView()override;
            Texture* texture()override;
            uint32_t mip()override;
            uint32_t baseLayer()override;
            uint32_t layerCount()override;
            GraphicsCard* graphicsCard()override;
            VkImageView vulkanHandle() const;
        private:
            void move(VulkanFrameBufferView& from);
            VulkanGraphicsCard* _graphicsCard=nullptr;
            Texture* _texture=nullptr;
            VkImageView _vulkanHandle=nullptr;
            uint32_t _mip=0;
            uint32_t _baseLayer=0;
            uint32_t _layerCount=0;

        };
    } // vulkan
} // slag

#endif //SLAG_VULKANFRAMEBUFFERVIEW_H