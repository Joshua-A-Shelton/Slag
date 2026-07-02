#ifndef SLAG_VULKANBUFFER_H
#define SLAG_VULKANBUFFER_H
#include <slag/Slag.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace slag
{
    namespace vulkan
    {
        class VulkanGraphicsCard;
        struct VulkanBufferMoveData
        {
            bool movedSucceded = false;
            VkBuffer buffer = nullptr;
        };

        const uint8_t VKBUFFER_MEMORY_BITS = 0b00000001;
        const uint8_t VKBUFFER_CPU_BITS    = 0b00000110;

        const uint8_t VKBUFFER_CPU_SHIFT = 1;

        class VulkanBuffer: public Buffer
        {
        public:
            VulkanBuffer(
                VulkanGraphicsCard* card,
                uint64_t size,
                BufferCPUAccess cpuAccess,
                BufferMemoryType memoryType);
            ~VulkanBuffer()override;
            VulkanBuffer(const VulkanBuffer&) = delete;
            VulkanBuffer& operator=(const VulkanBuffer&) = delete;
            VulkanBuffer(VulkanBuffer&& from) noexcept;
            VulkanBuffer& operator=(VulkanBuffer&& from) noexcept;

            [[nodiscard]] void* pointer()const override;
            [[nodiscard]] uint64_t deviceAddress()const override;
            [[nodiscard]] BufferMemoryType memoryType()const override;
            [[nodiscard]] BufferCPUAccess cpuAccess()const override;
            [[nodiscard]] uint64_t size()const override;
            [[nodiscard]] GraphicsCard* graphicsCard() const override;
            [[nodiscard]] void* userData() override;
            void setUserData(void* userData) override;

            [[nodiscard]] VkBuffer vulkanHandle()const;
            VulkanBufferMoveData moveMemory(VmaAllocation tempAllocation, CommandBuffer* copyDataBuffer);
            void updatePointer();

        private:
            void move(VulkanBuffer& from);
            MemoryReference _selfReference{.type = MemoryObjectType::BUFFER, .memory = {.buffer = this}};
            uint64_t _size = 0;
            VulkanGraphicsCard* _graphicsCard = nullptr;
            VkBuffer _buffer = nullptr;
            VmaAllocation _allocation = nullptr;
            void* _data = nullptr;
            void* _userData = nullptr;
            uint8_t _dataBits = 0;

        };
    } // vulkan
} // slag

#endif //SLAG_VULKANBUFFER_H
