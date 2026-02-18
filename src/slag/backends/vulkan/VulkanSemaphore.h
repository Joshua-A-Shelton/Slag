#ifndef SLAG_VULKANSEMAPHORE_H
#define SLAG_VULKANSEMAPHORE_H
#include <slag/Slag.h>
#include <vulkan/vulkan.h>
namespace slag
{
    namespace vulkan
    {
        class VulkanGraphicsCard;

        class VulkanSemaphore: public Semaphore
        {
        public:
            VulkanSemaphore(VulkanGraphicsCard* graphicsCard, uint64_t initialValue);
            VulkanSemaphore(VulkanSemaphore&)=delete;
            VulkanSemaphore& operator=(VulkanSemaphore&)=delete;
            VulkanSemaphore(VulkanSemaphore&& from) noexcept;
            VulkanSemaphore& operator=(VulkanSemaphore&& from) noexcept;
            ~VulkanSemaphore()override;
            uint64_t value()override;
            void signal(uint64_t value)override;
            void waitForValue(uint64_t value)override;
            [[nodiscard]] GraphicsCard* graphicsCard()const override;

            [[nodiscard]] VkSemaphore vulkanHandle()const;
        private:
            void move(VulkanSemaphore& from);
            VkSemaphore _semaphore = nullptr;
            VulkanGraphicsCard* _graphicsCard = nullptr;
        };
    } // vulkan
} // slag

#endif //SLAG_VULKANSEMAPHORE_H
