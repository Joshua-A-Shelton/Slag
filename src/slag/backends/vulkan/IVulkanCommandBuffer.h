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

            void begin() override;
            void end() override;
            void insertBarriers(GlobalBarrier* barriers, uint32_t barrierCount)override;
            void insertBarriers(BufferBarrier* barriers, uint32_t barrierCount)override;
            void insertBarriers(TextureBarrier* barriers, uint32_t barrierCount)override;
            void insertBarriers(
                GlobalBarrier* globalBarriers,
                uint32_t globalBarrierCount,
                BufferBarrier* bufferBarriers,
                uint32_t bufferBarrierCount,
                TextureBarrier* textureBarriers,
                uint32_t textureBarrierCount
                )override;
            void copyBufferToBuffer(Buffer* source, uint64_t sourceOffset, Buffer* destination, uint64_t destinationOffset, uint64_t length)override;
            void copyTextureToBuffer(Texture* source, Buffer* destination, TextureBufferMapping* copyData, uint32_t mappingCount)override;
            void copyBufferToTexture(Buffer* source, Texture* destination, TextureBufferMapping* copyData, uint32_t mappingCount)override;

            [[nodiscard]] VkCommandBuffer vulkanHandle() const;

        protected:
            void IVKCBMove(IVulkanCommandBuffer& from);
            VkCommandBuffer _commandBuffer = nullptr;
            VkCommandPool _commandPool = nullptr;
            VulkanGraphicsCard* _graphicsCard = nullptr;
            QueueType _type = QueueType::TRANSFER;
        };
    } // vulkan
} // slag

#endif //SLAG_IVULKANCOMMANDBUFFER_H
