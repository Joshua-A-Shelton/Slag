#ifndef SLAG_IVULKANCOMMANDBUFFER_H
#define SLAG_IVULKANCOMMANDBUFFER_H
#include <slag/Slag.h>
#include <vulkan/vulkan.h>
namespace slag
{
    namespace vulkan
    {
        class VulkanGraphicsCard;
        class IVulkanCommandBuffer: public CommandBuffer
        {
        public:
            ~IVulkanCommandBuffer()override = default;
            [[nodiscard]] GraphicsCard* graphicsCard() override;
            [[nodiscard]] QueueType type()const override;
            [[nodiscard]] CommandBufferLevel level()const override;

            void begin() override;
            void end() override;
            void copyBufferToBuffer(Buffer* source, uint64_t sourceOffset, Buffer* destination, uint64_t destinationOffset, uint64_t length)override;
            void copyTextureToBuffer(Texture* source, Buffer* destination, TextureBufferMapping* copyData, uint32_t mappingCount)override;
            void copyBufferToTexture(Buffer* source, Texture* destination, TextureBufferMapping* copyData, uint32_t mappingCount)override;

            [[nodiscard]] VkCommandBuffer vulkanHandle() const;

        protected:
            VkCommandBuffer _commandBuffer = nullptr;
            VkCommandPool _commandPool = nullptr;
            VulkanGraphicsCard* _graphicsCard = nullptr;
            QueueType _type = QueueType::TRANSFER;
            CommandBufferLevel _level = CommandBufferLevel::PRIMARY;
        };
    } // vulkan
} // slag

#endif //SLAG_IVULKANCOMMANDBUFFER_H
