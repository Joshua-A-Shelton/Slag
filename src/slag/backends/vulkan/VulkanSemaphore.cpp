#include "VulkanSemaphore.h"

#include "VulkanGraphicsCard.h"
#include "slag/exceptions/ResourceCreationError.h"

namespace slag
{
    namespace vulkan
    {
        VulkanSemaphore::VulkanSemaphore(VulkanGraphicsCard* graphicsCard, uint64_t initialValue)
        {
            _graphicsCard = graphicsCard;

            VkSemaphoreTypeCreateInfo semaphoreTypeCreateInfo{};
            semaphoreTypeCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
            semaphoreTypeCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
            semaphoreTypeCreateInfo.initialValue = initialValue;

            VkSemaphoreCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            createInfo.pNext = &semaphoreTypeCreateInfo;

            if (vkCreateSemaphore( graphicsCard->device(),&createInfo, nullptr,&_semaphore)!= VK_SUCCESS)
            {
                throw ResourceCreationError("Unable to create Semaphore");
            }
        }

        VulkanSemaphore::VulkanSemaphore(VulkanSemaphore&& from) noexcept
        {
            move(from);
        }

        VulkanSemaphore& VulkanSemaphore::operator=(VulkanSemaphore&& from) noexcept
        {
            move(from);
            return *this;
        }

        VulkanSemaphore::~VulkanSemaphore()
        {
            if (_semaphore != nullptr)
            {
                vkDestroySemaphore( _graphicsCard->device(),_semaphore,nullptr);
            }
        }

        uint64_t VulkanSemaphore::value()
        {
            uint64_t value;
            vkGetSemaphoreCounterValue(_graphicsCard->device(),_semaphore,&value);
            return value;
        }

        void VulkanSemaphore::signal(uint64_t value)
        {
            VkSemaphoreSignalInfo signalInfo{};
            signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
            signalInfo.semaphore = _semaphore;
            signalInfo.value = value;

            vkSignalSemaphore(_graphicsCard->device(), &signalInfo);
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

            vkWaitSemaphores(_graphicsCard->device(), &waitInfo, UINT64_MAX);
        }

        GraphicsCard* VulkanSemaphore::graphicsCard() const
        {
            return _graphicsCard;
        }

        VkSemaphore VulkanSemaphore::vulkanHandle() const
        {
            return _semaphore;
        }

        void VulkanSemaphore::move(VulkanSemaphore& from)
        {
            std::swap(_semaphore, from._semaphore);
            std::swap(_graphicsCard, from._graphicsCard);
        }
    } // vulkan
} // slag
