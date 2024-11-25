#include "VulkanSemaphore.h"
#include "VulkanLib.h"
namespace slag
{
    namespace vulkan
    {
        VulkanSemaphore::VulkanSemaphore(uint64_t initialValue, bool destroyImmediately): resources::Resource(destroyImmediately)
        {
            VkSemaphoreTypeCreateInfo semaphoreTypeCreateInfo{};
            semaphoreTypeCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
            semaphoreTypeCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
            semaphoreTypeCreateInfo.initialValue = initialValue;

            VkSemaphoreCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            createInfo.pNext = &semaphoreTypeCreateInfo;

            vkCreateSemaphore(VulkanLib::card()->device(),&createInfo, nullptr,&_semaphore);
            auto semaphore = _semaphore;
            _disposeFunction = [=]
            {
                vkDestroySemaphore(VulkanLib::card()->device(), semaphore, nullptr);
            };
        }

        VulkanSemaphore::~VulkanSemaphore()
        {
            if(_semaphore)
            {
                smartDestroy();
            }
        }

        VulkanSemaphore::VulkanSemaphore(slag::vulkan::VulkanSemaphore&& from): resources::Resource(from._destroyImmediately)
        {
            resources::Resource::move(from);
            _semaphore = from._semaphore;
            from._semaphore = nullptr;
        }
        VulkanSemaphore& VulkanSemaphore::operator=(slag::vulkan::VulkanSemaphore&& from)
        {
            _semaphore = from._semaphore;
            from._semaphore = nullptr;
            resources::Resource::move(from);
            return *this;
        }

        uint64_t VulkanSemaphore::value()
        {
            uint64_t value;
            vkGetSemaphoreCounterValue(VulkanLib::card()->device(),_semaphore,&value);
            return value;
        }

        void VulkanSemaphore::signal(uint64_t value)
        {
            VkSemaphoreSignalInfo signalInfo{};
            signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
            signalInfo.semaphore = _semaphore;
            signalInfo.value = value;

            vkSignalSemaphore(VulkanLib::card()->device(), &signalInfo);
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

            vkWaitSemaphores(VulkanLib::card()->device(), &waitInfo, UINT64_MAX);
        }

        VkSemaphore VulkanSemaphore::semaphore()
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

            vkWaitSemaphores(VulkanLib::card()->device(), &waitInfo, UINT64_MAX);
        }
    } // vulkan
} // slag