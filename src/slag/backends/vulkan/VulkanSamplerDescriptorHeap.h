#ifndef SLAG_VULKANSAMPLERDESCRIPTORHEAP_H
#define SLAG_VULKANSAMPLERDESCRIPTORHEAP_H
#include <vk_mem_alloc.h>
#include <slag/Slag.h>
namespace slag
{
    namespace vulkan
    {
        class VulkanGraphicsCard;

        class VulkanSamplerDescriptorHeap: public SamplerDescriptorHeap
        {
        public:
            VulkanSamplerDescriptorHeap(VulkanGraphicsCard* card, uint32_t descriptorCount);
            ~VulkanSamplerDescriptorHeap() override;
            VulkanSamplerDescriptorHeap(const VulkanSamplerDescriptorHeap&)=delete;
            VulkanSamplerDescriptorHeap& operator=(const VulkanSamplerDescriptorHeap&)=delete;
            VulkanSamplerDescriptorHeap(VulkanSamplerDescriptorHeap&& from) noexcept;
            VulkanSamplerDescriptorHeap& operator=(VulkanSamplerDescriptorHeap&& from) noexcept;
            GraphicsCard* graphicsCard() override;
            uint32_t descriptorCount() override;
            void setSampler(uint32_t heapOffset, Sampler* sampler) override;
            [[nodiscard]] VkDeviceAddress deviceAddress() const;
            [[nodiscard]] uint64_t size() const;
            [[nodiscard]] uint64_t reserved() const;

        private:
            void move(VulkanSamplerDescriptorHeap& from);
            VulkanGraphicsCard* _card=nullptr;
            VkBuffer _buffer = nullptr;
            VmaAllocation _allocation = nullptr;
            void* _data = nullptr;
            VkDeviceAddress _deviceAddress = 0;
            uint64_t _reserved = 0;
            uint32_t _descriptorCount = 0;
            uint32_t _descriptorSize = 0;

        };
    } // vulkan
} // slag

#endif //SLAG_VULKANSAMPLERDESCRIPTORHEAP_H