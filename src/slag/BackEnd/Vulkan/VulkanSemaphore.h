#ifndef SLAG_VULKANFENCE_H
#define SLAG_VULKANFENCE_H
#include "../../Semaphore.h"
#include "../../Resources/Resource.h"
#include <vulkan/vulkan.h>
namespace slag
{
    namespace vulkan
    {

        class VulkanSemaphore: public Semaphore, resources::Resource
        {
        public:
            VulkanSemaphore(uint64_t initialValue, bool destroyImmediately = false);
            ~VulkanSemaphore();
            VulkanSemaphore(const VulkanSemaphore&)=delete;
            VulkanSemaphore& operator=(const VulkanSemaphore&)=delete;
            VulkanSemaphore(VulkanSemaphore&& from);
            VulkanSemaphore& operator=(VulkanSemaphore&& from);
            void* gpuID()override;
            uint64_t value()override;
            void signal(uint64_t value)override;
            void waitForValue(uint64_t value)override;

            static void waitFor(SemaphoreValue* values, size_t count);
            VkSemaphore semaphore();
        private:
            VkSemaphore _semaphore = nullptr;
        };

    } // vulkan
} // slag

#endif //SLAG_VULKANFENCE_H
