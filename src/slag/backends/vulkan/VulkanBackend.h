#ifndef SLAG_VULKANBACKEND_H
#define SLAG_VULKANBACKEND_H

#include <vector>

#include "VulkanGraphicsCard.h"
#include "slag/core/IBackend.h"
namespace slag
{
    namespace vulkan
    {
        struct NativeFormat
        {
            VkFormat format = VK_FORMAT_UNDEFINED;
            VkComponentSwizzle rSwizzle = VK_COMPONENT_SWIZZLE_IDENTITY;
            VkComponentSwizzle gSwizzle = VK_COMPONENT_SWIZZLE_IDENTITY;
            VkComponentSwizzle bSwizzle = VK_COMPONENT_SWIZZLE_IDENTITY;
            VkComponentSwizzle aSwizzle = VK_COMPONENT_SWIZZLE_IDENTITY;
        };
        class VulkanBackend: public IBackend
        {
        public:
            VulkanBackend();
            ~VulkanBackend()override;
            [[nodiscard]] BackendAPI api()const override;
            [[nodiscard]] uint32_t graphicsCardCount()const override;
            [[nodiscard]] GraphicsCard* graphicsCard(uint32_t index)override;

            static VkBufferUsageFlagBits2 nativeBufferUsage(BufferMemoryType access);
            static NativeFormat nativeFormat(PixelFormat format);
            static VkImageUsageFlags nativeTextureUsage(TextureUsageFlags flags);
            inline static VkImageAspectFlags nativeTextureAspect(PixelAspectFlags aspect){return static_cast<VkImageAspectFlagBits>(aspect);}
        private:
            SlagInitializationResult initializeBackend(const InitializationData& initializationData)override;
            std::vector<VulkanGraphicsCard> _graphicsCards;
            vkb::Instance _instance{};
            VkDebugUtilsMessengerEXT _debugMessenger=nullptr;
        };
    } // vulkan
} // slag

#endif //SLAG_VULKANBACKEND_H
