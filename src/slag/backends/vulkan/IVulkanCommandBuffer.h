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
            void bindDescriptorHeaps(ResourceDescriptorHeap* resourceHeap, SamplerDescriptorHeap* samplerHeap)override;
            void setGraphicsShaderParameters(uint32_t shaderDataOffset, void* data, uint32_t dataSize)override;
            void setComputeShaderParameters(uint32_t shaderDataOffset, void* data, uint32_t dataSize)override;
            void copyBufferToBuffer(Buffer* source, uint64_t sourceOffset, Buffer* destination, uint64_t destinationOffset, uint64_t length)override;
            void copyTextureToBuffer(Texture* source, Buffer* destination, TextureBufferMapping* copyData, uint32_t mappingCount)override;
            void copyBufferToTexture(Buffer* source, Texture* destination, TextureBufferMapping* copyData, uint32_t mappingCount)override;
            void bindShaderPipeline(ShaderPipeline* pipeline)override;
            void beginRendering(Attachment* colorAttachments, uint32_t colorAttachmentCount,Attachment* depthAttachment, const Rectangle& bounds)override;
            void endRendering()override;
            void setViewPort(float x, float y, float width, float height, float minDepth, float maxDepth)override;
            void setScissors(const Rectangle& rect)override;
            void bindIndexBuffer(Buffer* buffer, IndexBufferType indexType, uint64_t offset)override;
            void bindVertexBuffers(uint32_t firstBinding, Buffer** buffers, uint64_t* bufferOffsets, uint64_t* strides, uint32_t bufferCount)override;
            void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)override;
            void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)override;
            void drawIndirect(Buffer* buffer, uint64_t offset, uint32_t drawCount, uint32_t stride)override;
            void drawIndexedIndirect(Buffer* buffer, uint64_t offset, uint32_t drawCount, uint32_t stride)override;
            void drawIndirectCount(Buffer* buffer, uint64_t offset, Buffer* countBuffer, uint64_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride)override;
            void drawIndexedIndirectCount(Buffer* buffer, uint64_t offset, Buffer* countBuffer, uint64_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride)override;
            void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)override;
            void dispatchIndirect(Buffer* buffer, uint64_t offset)override;

            [[nodiscard]] VkCommandBuffer vulkanHandle() const;

        protected:
            void IVKCBMove(IVulkanCommandBuffer& from);
            VkCommandBuffer _commandBuffer = nullptr;
            VkCommandPool _commandPool = nullptr;
            VulkanGraphicsCard* _graphicsCard = nullptr;
            QueueType _type = QueueType::TRANSFER;
#ifdef SLAG_DEBUG
            bool _inRenderPass = false;
            bool _setViewport = false;
            bool _setScissor = false;
            enum class BoundPipeLineType
            {
                NONE,
                GRAPHICS,
                COMPUTE
            };
            BoundPipeLineType _boundPipelineType = BoundPipeLineType::NONE;
#endif

        };
    } // vulkan
} // slag

#endif //SLAG_IVULKANCOMMANDBUFFER_H
