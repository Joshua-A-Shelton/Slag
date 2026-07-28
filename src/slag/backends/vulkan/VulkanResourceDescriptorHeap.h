#ifndef SLAG_VULKANRESOURCEDESCRIPTORHEAP_H
#define SLAG_VULKANRESOURCEDESCRIPTORHEAP_H
#include <vk_mem_alloc.h>
#include <slag/Slag.h>
namespace slag
{
    namespace vulkan
    {
        class VulkanGraphicsCard;

        class VulkanResourceDescriptorHeap: public ResourceDescriptorHeap
        {
        public:
            VulkanResourceDescriptorHeap(VulkanGraphicsCard* card, uint32_t descriptorCount);
            ~VulkanResourceDescriptorHeap() override;
            VulkanResourceDescriptorHeap(const VulkanResourceDescriptorHeap&)=delete;
            VulkanResourceDescriptorHeap& operator=(const VulkanResourceDescriptorHeap&)=delete;
            VulkanResourceDescriptorHeap(VulkanResourceDescriptorHeap&& from) noexcept;
            VulkanResourceDescriptorHeap& operator=(VulkanResourceDescriptorHeap&& from) noexcept;
            GraphicsCard* graphicsCard() override;
            [[nodiscard]] uint64_t size()override;

            virtual void* data()override;
            virtual uint64_t deviceAddress()override;

            [[nodiscard]] uint64_t reserved() const;
        private:
            void move(VulkanResourceDescriptorHeap& from);
            VulkanGraphicsCard* _card=nullptr;
            VkBuffer _buffer = nullptr;
            VmaAllocation _allocation = nullptr;
            void* _data = nullptr;
            uint64_t _reserved = 0;
            VkDeviceAddress _deviceAddress = 0;
            uint32_t _descriptorCount = 0;
            uint32_t _textureDescriptorSize = 0;
            uint32_t _bufferDescriptorSize = 0;
        };
    } // vulkan
} // slag

#endif //SLAG_VULKANRESOURCEDESCRIPTORHEAP_H