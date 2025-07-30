#ifndef SLAG_VULKANSEMAPHORE_H
#define SLAG_VULKANSEMAPHORE_H
#include <slag/Slag.h>
#include <vulkan/vulkan.h>
namespace slag
{
    namespace vulkan
    {
        class VulkanSemaphore: public Semaphore
        {
        public:
            VulkanSemaphore(uint64_t initialValue);
            virtual ~VulkanSemaphore()override;
            virtual uint64_t value()override;
            virtual void signal(uint64_t value)override;
            virtual void waitForValue(uint64_t value)override;
            VkSemaphore vulkanSemaphore() const;
            static void waitFor(SemaphoreValue* values, size_t count);
        private:
            VkSemaphore _semaphore;
        };
    } // vulkan
} // slag

#endif //SLAG_VULKANSEMAPHORE_H
