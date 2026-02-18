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
        const uint8_t VKBUFFER_USAGE_BITS  = 0b00000111;
        const uint8_t VKBUFFER_SHADER_BITS = 0b00001000;
        const uint8_t VKBUFFER_CPU_BITS    = 0b00110000;

        const uint8_t VKBUFFER_SHADER_SHIFT = 3;
        const uint8_t VKBUFFER_CPU_SHIFT = 4;

        class VulkanBuffer: public Buffer
        {
        public:
            VulkanBuffer(
                VulkanGraphicsCard* card,
                uint64_t size,
                BufferUsage usage,
                BufferShaderAccess shaderAccess,
                BufferCPUAccess cpuAccess);
            ~VulkanBuffer()override;
            VulkanBuffer(const VulkanBuffer&) = delete;
            VulkanBuffer& operator=(const VulkanBuffer&) = delete;
            VulkanBuffer(VulkanBuffer&& from) noexcept;
            VulkanBuffer& operator=(VulkanBuffer&& from) noexcept;

            [[nodiscard]] void* data()const override;
            [[nodiscard]] uint64_t deviceAddress()const override;
            [[nodiscard]] BufferUsage usage()const override;
            [[nodiscard]] BufferShaderAccess shaderAccess()const override;
            [[nodiscard]] BufferCPUAccess cpuAccess()const override;
            [[nodiscard]] uint64_t size()const override;
            [[nodiscard]] GraphicsCard* graphicsCard() const override;

            [[nodiscard]] VkBuffer vulkanHandle()const;

        private:
            void move(VulkanBuffer& from);
            uint64_t _size = 0;
            VulkanGraphicsCard* _graphicsCard = nullptr;
            VkBuffer _buffer = nullptr;
            VmaAllocation _allocation = nullptr;
            void* _data = nullptr;
            uint8_t _dataBits = 0;

        };
    } // vulkan
} // slag

#endif //SLAG_VULKANBUFFER_H
