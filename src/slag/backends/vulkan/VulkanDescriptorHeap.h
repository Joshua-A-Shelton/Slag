#ifndef SLAG_VULKANDESCRIPTORHEAP_H
#define SLAG_VULKANDESCRIPTORHEAP_H
#include <vk_mem_alloc.h>
#include <slag/Slag.h>
#include <vulkan/vulkan.h>

namespace slag
{
    namespace vulkan
    {
        class VulkanGraphicsCard;

        class VulkanDescriptorHeap: public DescriptorHeap
        {
        public:
            VulkanDescriptorHeap(VulkanGraphicsCard* graphicsCard, DescriptorHeapType type, uint32_t size);
            VulkanDescriptorHeap(const VulkanDescriptorHeap&)=delete;
            VulkanDescriptorHeap& operator=(const VulkanDescriptorHeap&)=delete;
            VulkanDescriptorHeap(VulkanDescriptorHeap&& from) noexcept;
            VulkanDescriptorHeap& operator=(VulkanDescriptorHeap&& from) noexcept;
            ~VulkanDescriptorHeap()override;
            void writeSamplerDescriptor(uint32_t location, Sampler* sampler)override;
            void writeSampledTextureDescriptor(uint32_t location, Texture* texture)override;
            void writeUnorderedAccessTextureDescriptor(uint32_t location, Texture* texture)override;
            void writeUniformBufferDescriptor(uint32_t location, Buffer* buffer, uint64_t offset, uint64_t length)override;
            void writeUnorderedAccessBufferDescriptor(uint32_t location, Buffer* buffer, uint64_t offset, uint64_t length)override;
            void writeUniformTexelBufferDescriptor(uint32_t location, Buffer* buffer, uint64_t offset, uint64_t length, PixelFormat format)override;
            void writeUnorderedAccessTexelBufferDescriptor(uint32_t location, Buffer* buffer, uint64_t offset, uint64_t length, PixelFormat format)override;

            GraphicsCard* graphicsCard()override;
            uint32_t size() override;
            uint32_t reserved() override;
            DescriptorHeapType type()override;
            VkDeviceAddress deviceAddress() const;
        private:
            void move(VulkanDescriptorHeap& from);
            uint32_t _size = 0;
            uint32_t _reserved = 0;
            VulkanGraphicsCard* _graphicsCard = nullptr;
            VkBuffer _buffer = nullptr;
            VmaAllocation _allocation = nullptr;
            void* _data = nullptr;
            DescriptorHeapType _type = DescriptorHeapType::RESOURCE;
            VkDeviceAddress _deviceAddress = 0;
        };
    } // vulkan
} // slag

#endif //SLAG_VULKANDESCRIPTORHEAP_H