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

            virtual void* pointer()override;
            virtual uint64_t deviceAddress()override;

            void setUniformTexture(uint64_t heapOffset, Texture* texture, uint32_t baseMip, uint32_t mipCount, uint32_t baseLayer, uint32_t layerCount)override;
            void setUnorderedAccessTexture(uint64_t heapOffset, Texture* texture, uint32_t mip, uint32_t baseLayer, uint32_t layerCount)override;
            void setUniformBuffer(uint64_t heapOffset, Buffer* buffer, uint32_t offset, uint32_t length)override;
            void setStorageBuffer(uint64_t heapOffset, Buffer* buffer, uint64_t firstElementIndex, uint64_t elementCount, uint64_t elementStride)override;
            void setUniformTexelBuffer(uint64_t heapOffset, Buffer* buffer, PixelFormat format, uint64_t offset, uint64_t length)override;
            void setStorageTexelBuffer(uint64_t heapOffset, Buffer* buffer, PixelFormat format, uint64_t offset, uint64_t length)override;

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