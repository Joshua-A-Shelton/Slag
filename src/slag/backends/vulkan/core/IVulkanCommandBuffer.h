#ifndef SLAG_IVULKANCOMMANDBUFFER_H
#define SLAG_IVULKANCOMMANDBUFFER_H

#include <slag/Slag.h>
#include <vulkan/vulkan.h>

#include "VulkanShaderPipeline.h"

namespace slag
{
    namespace vulkan
    {
        class IVulkanCommandBuffer : public CommandBuffer
        {
        public:
            virtual ~IVulkanCommandBuffer() override =default;

            virtual GPUQueue::QueueType commandType() override;
            virtual void begin() override;
            virtual void end() override;

            virtual void insertBarriers(TextureBarrier* textureBarriers, uint32_t textureBarrierCount, BufferBarrier* bufferBarriers, uint32_t
                                        bufferBarrierCount, GlobalBarrier* memoryBarriers, uint32_t memoryBarrierCount) override;
            virtual void insertBarrier(const TextureBarrier& barrier) override;
            virtual void clearTexture(Texture* texture, ClearColor color) override;
            virtual void clearTexture(Texture* texture, ClearDepthStencilValue depthStencil) override;
            virtual void updateMip(Texture* texture, uint32_t layer, uint32_t sourceMip, uint32_t destinationMip) override;
            virtual void copyTextureToBuffer(Texture* source, Buffer* destination, TextureBufferMapping* mappings, uint32_t mappingCount) override;
            virtual void copyBufferToTexture(Buffer* source, Texture* destination,TextureBufferMapping* mappings, uint32_t mappingCount)override;
            virtual void blit(Texture* source, uint32_t sourceLayer, uint32_t sourceMip, Rectangle sourceArea,Texture* destination, uint32_t destinationLayer, uint32_t destinationMip,Rectangle destinationArea) override;
            virtual void blit(Texture* source, uint32_t sourceLayer, uint32_t sourceMip, Rectangle sourceArea,Texture* destination, uint32_t destinationLayer, uint32_t destinationMip,Rectangle destinationArea, Pixels::AspectFlags aspects) override;
            virtual void resolve(Texture* source, uint32_t sourceLayer, uint32_t sourceMip, Offset sourceOffset, Texture* destination, uint32_t destinationLayer, uint32_t destinationMip, Offset destinationOffset, Extent resolveExtent) override;
            virtual void beginRendering(Attachment* colorAttachments, uint32_t colorAttachmentCount, Attachment* depthAttachment, Rectangle bounds) override;
            virtual void insertBarrier(const BufferBarrier& barrier) override;
            virtual void insertBarrier(const GlobalBarrier& barrier) override;

            virtual void copyBufferToBuffer(Buffer* source, uint64_t sourceOffset, uint64_t length, Buffer* destination,uint64_t destinationOffset) override;

            virtual void fillBuffer(Buffer* buffer, uint64_t offset, uint64_t length, uint32_t data) override;
            virtual void setViewPort(float x, float y, float width, float height, float minDepth, float maxDepth) override;
            virtual void setScissors(Rectangle rectangle) override;
            virtual void setBlendConstants(float r, float g, float b, float a) override;
            virtual void setStencilReference(uint32_t reference) override;
            virtual void endRendering() override;
            virtual void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,uint32_t firstInstance) override;
            virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,int32_t vertexOffset, uint32_t firstInstance) override;
            virtual void drawIndexedIndirect(Buffer* buffer, uint64_t offset, uint32_t drawCount, uint32_t stride) override;
            virtual void drawIndexedIndirectCount(Buffer* buffer, uint64_t offset, Buffer* countBuffer,uint64_t countBufferOffset, uint32_t maxDrawCount,uint32_t stride) override;
            virtual void drawIndirect(Buffer* buffer, uint64_t offset, uint32_t drawCount, uint32_t stride) override;
            virtual void drawIndirectCount(Buffer* buffer, uint64_t offset, Buffer* countBuffer,uint64_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride) override;
            virtual void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override;
            virtual void dispatchBase(uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ,uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override;
            virtual void dispatchIndirect(Buffer* buffer, uint64_t offset) override;
            virtual void bindGraphicsShaderPipeline(ShaderPipeline* pipeline) override;
            virtual void bindComputeShaderPipeline(ShaderPipeline* pipeline) override;
            virtual void bindGraphicsDescriptorBundle(uint32_t index, DescriptorBundle& bundle) override;
            virtual void bindComputeDescriptorBundle(uint32_t index, DescriptorBundle& bundle) override;
            virtual void pushGraphicsConstants(uint32_t offset, uint32_t size, void* data)override;
            virtual void pushComputeConstants(uint32_t offset, uint32_t size, void* data)override;
            virtual void bindIndexBuffer(Buffer* buffer, Buffer::IndexSize indexSize, uint64_t offset) override;
            virtual void bindVertexBuffers(uint32_t firstBindingIndex, Buffer** buffers, uint64_t* bufferOffsets, uint64_t* strides, uint32_t bufferCount) override;


            void transitionToLayout(Texture* texture,VkImageLayout oldLayout, VkImageLayout newLayout,VkAccessFlags2 accessBefore, VkAccessFlags2 accessAfter, VkPipelineStageFlags2 syncBefore, VkPipelineStageFlags2 syncAfter) const;
            VkCommandBuffer vulkanCommandBufferHandle();
            VkCommandPool vulkanCommandPoolHandle();

        protected:
            VkCommandBuffer _commandBuffer = nullptr;
            VkCommandPool _pool = nullptr;
            GPUQueue::QueueType _type = GPUQueue::QueueType::GRAPHICS;
            VkPipelineLayout _boundVulkanGraphicsShaderPipelineLayout = nullptr;
            VkPipelineLayout _boundVulkanComputePipelineLayout = nullptr;
#ifdef SLAG_DEBUG
            DescriptorPool* _boundDescriptorPool = nullptr;
            bool _inRenderPass = false;
            bool _setViewport = false;
            bool _setScissor = false;
#endif
        };
    } // vulkan
} // slag

#endif //SLAG_IVULKANCOMMANDBUFFER_H
