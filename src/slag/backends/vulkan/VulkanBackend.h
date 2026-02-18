#ifndef SLAG_VULKANBACKEND_H
#define SLAG_VULKANBACKEND_H

#include <vector>

#include "VulkanGraphicsCard.h"
#include "slag/core/IBackend.h"
namespace slag
{
    namespace vulkan
    {
        class VulkanBackend: public IBackend
        {
        public:
            VulkanBackend();
            ~VulkanBackend()override;
            [[nodiscard]] BackendAPI api()const override;
            [[nodiscard]] uint32_t graphicsCardCount()const override;
            [[nodiscard]] GraphicsCard* graphicsCard(uint32_t index)override;

            static VkBufferUsageFlagBits2 nativeBufferUsage(BufferUsage usage,BufferShaderAccess access);
        private:
            SlagInitializationResult initializeBackend(const InitializationData& initializationData)override;
            std::vector<VulkanGraphicsCard> _graphicsCards;
            vkb::Instance _instance{};
            VkDebugUtilsMessengerEXT _debugMessenger=nullptr;
        };
    } // vulkan
} // slag

#endif //SLAG_VULKANBACKEND_H
