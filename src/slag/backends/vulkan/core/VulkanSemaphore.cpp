#include "VulkanSemaphore.h"

#include "VulkanGraphicsCard.h"
#include "VulkanGraphicsCard.h"

namespace slag
{
    namespace vulkan
    {
        VulkanSemaphore::VulkanSemaphore(uint64_t initialValue)
        {
            VkSemaphoreTypeCreateInfo semaphoreTypeCreateInfo{};
            semaphoreTypeCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
            semaphoreTypeCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
            semaphoreTypeCreateInfo.initialValue = initialValue;

            VkSemaphoreCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            createInfo.pNext = &semaphoreTypeCreateInfo;

            vkCreateSemaphore( VulkanGraphicsCard::selected()->device(),&createInfo, nullptr,&_semaphore);
        }

        VulkanSemaphore::~VulkanSemaphore()
        {
            vkDestroySemaphore( VulkanGraphicsCard::selected()->device(),_semaphore,nullptr);
        }

        uint64_t VulkanSemaphore::value()
        {
            uint64_t value;
            vkGetSemaphoreCounterValue(VulkanGraphicsCard::selected()->device(),_semaphore,&value);
            return value;
        }

        void VulkanSemaphore::signal(uint64_t value)
        {
            VkSemaphoreSignalInfo signalInfo{};
            signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
            signalInfo.semaphore = _semaphore;
            signalInfo.value = value;

            vkSignalSemaphore(VulkanGraphicsCard::selected()->device(), &signalInfo);
        }

        void VulkanSemaphore::waitForValue(uint64_t value)
        {
            VkSemaphoreWaitInfo waitInfo;
            waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
            waitInfo.pNext = nullptr;
            waitInfo.flags = 0;
            waitInfo.semaphoreCount = 1;
            waitInfo.pSemaphores = &_semaphore;
            waitInfo.pValues = &value;

            vkWaitSemaphores(VulkanGraphicsCard::selected()->device(), &waitInfo, UINT64_MAX);
        }

        VkSemaphore VulkanSemaphore::vulkanSemaphore() const
        {
            return _semaphore;
        }

        void VulkanSemaphore::waitFor(SemaphoreValue* values, size_t count)
        {
            std::vector<VkSemaphore> semaphores(count);
            std::vector<uint64_t> waitValues(count);
            for(auto i=0; i< count; i++)
            {
                semaphores[i] = static_cast<VulkanSemaphore*>(values[i].semaphore)->_semaphore;
                waitValues[i] = values[i].value;
            }

            VkSemaphoreWaitInfo waitInfo;
            waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
            waitInfo.pNext = nullptr;
            waitInfo.flags = 0;
            waitInfo.semaphoreCount = count;
            waitInfo.pSemaphores = semaphores.data();
            waitInfo.pValues = waitValues.data();

            vkWaitSemaphores(VulkanGraphicsCard::selected()->device(), &waitInfo, UINT64_MAX);
        }
    } // vulkan
} // slag
