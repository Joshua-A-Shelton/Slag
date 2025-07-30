#ifndef SLAG_IVULKANCOMMANDBUFFER_H
#define SLAG_IVULKANCOMMANDBUFFER_H

#include <slag/Slag.h>
#include <vulkan/vulkan.h>

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

#ifndef SLAG_DISCREET_TEXTURE_LAYOUTS
            virtual void insertBarriers(TextureBarrier* textureBarriers, size_t textureBarrierCount,BufferBarrier* bufferBarriers, size_t bufferBarrierCount,GlobalBarrier* memoryBarriers, size_t memoryBarrierCount) override;
            virtual void insertBarrier(const TextureBarrier& barrier) override;
            virtual void clearTexture(Texture* texture, ClearColor color) override;
            virtual void clearTexture(Texture* texture, ClearDepthStencilValue depthStencil) override;
            virtual void updateMip(Texture* texture, uint32_t layer, uint32_t sourceMip, uint32_t destinationMip) override;
            virtual void copyTextureToBuffer(Texture* source, TextureToBufferCopyData* copyData,uint32_t subresourceCount, Buffer* destination) override;
            virtual void copyBufferToTexture(Buffer* source, uint64_t offset, Texture* destination,TextureSubresource subresource)override;
            virtual void blit(Texture* source, uint32_t sourceLayer, uint32_t sourceMip, Rectangle sourceArea,Texture* destination, uint32_t destinationLayer, uint32_t destinationMip,Rectangle destinationArea) override;
            virtual void blit(Texture* source, uint32_t sourceLayer, uint32_t sourceMip, Rectangle sourceArea,Texture* destination, uint32_t destinationLayer, uint32_t destinationMip,Rectangle destinationArea, Pixels::AspectFlags aspects) override;
            virtual void resolve(Texture* source, uint32_t sourceLayer, uint32_t sourceMip, Offset sourceOffset, Texture* destination, uint32_t destinationLayer, uint32_t destinationMip, Offset destinationOffset, Extent resolveExtent) override;
            virtual void beginRendering(Attachment* colorAttachments, size_t colorAttachmentCount,Attachment* depthAttachment, Rectangle bounds) override;

#else
            virtual void insertBarriers(TextureBarrierDiscreet* textureBarriers, size_t textureBarrierCount, BufferBarrier* bufferBarriers, size_t bufferBarrierCount, GPUMemoryBarrier* memoryBarriers, size_t memoryBarrierCount)override;
            virtual void insertBarrier(const TextureBarrierDiscreet& barrier)override;
            virtual void clearTexture(Texture* texture,TextureLayouts::Layout textureLayout, ClearColor color)override;
            virtual void clearTexture(Texture* texture,TextureLayouts::Layout textureLayout ClearDepthStencilValue depthStencil)override;
            virtual void updateMip(Texture* texture, uint32_t layer, uint32_t sourceMip,TextureLayouts::Layout sourceLayout, uint32_t destinationMip, TextureLayouts::Layout destinationLayout)override;
            virtual void copyTextureToBuffer(Texture* source, TextureLayouts::Layout textureLayout, TextureSubresource* subresources, size_t subresourceCount, Buffer* destination)override;
            virtual void copyBufferToTexture(Buffer* source, uint64_t offset, Texture* destination,TextureSubresource subresource,TextureLayouts::Layout destinationLayout)override;
            virtual void blit(Texture* source,TextureLayouts::Layout sourceLayout,uint32_t sourceLayer, uint32_t sourceMip,Rectangle sourceArea, Texture* destination, TextureLayouts::Layout destinationLayout,uint32_t destinationLayer, uint32_t destinationMip,Rectangle destinationArea)override;
            virtual void blit(Texture* source,TextureLayouts::Layout sourceLayout,uint32_t sourceLayer, uint32_t sourceMip,Rectangle sourceArea, Texture* destination, TextureLayouts::Layout destinationLayout,uint32_t destinationLayer, uint32_t destinationMip,Rectangle destinationArea,Pixels::AspectFlags aspects)override;
            virtual void resolve(Texture* source, TextureLayouts::Layout sourceLayout, uint32_t sourceLayer, uint32_t sourceMip, Offset sourceOffset, Texture* destination, TextureLayouts::Layout destinationLayout, uint32_t destinationLayer, uint32_t destinationMip, Offset destinationOffset, Extent resolveExtent)override;
            virtual void beginRendering(AttachmentDiscreet* colorAttachments, size_t colorAttachmentCount,AttachmentDiscreet* depthAttachment,Rectangle bounds)override;
#endif
            virtual void insertBarrier(const BufferBarrier& barrier) override;
            virtual void insertBarrier(const GlobalBarrier& barrier) override;

            virtual void copyBufferToBuffer(Buffer* source, uint64_t sourceOffset, uint64_t length, Buffer* destination,uint64_t destinationOffset) override;

            virtual void fillBuffer(Buffer* buffer, size_t offset, size_t length, uint32_t data) override;
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
            virtual void dispatchIndirect(Buffer* buffer, size_t offset) override;
            virtual void bindGraphicsShaderPipeline(ShaderPipeline* pipeline) override;
            virtual void bindComputeShaderPipeline(ShaderPipeline* pipeline) override;
            virtual void bindGraphicsDescriptorBundle(uint32_t index, DescriptorBundle& bundle) override;
            virtual void bindComputeDescriptorBundle(uint32_t index, DescriptorBundle& bundle) override;
            virtual void bindIndexBuffer(Buffer* buffer, Buffer::IndexSize indexSize, uint64_t offset) override;
            virtual void bindVertexBuffers(uint32_t firstBindingIndex, Buffer** buffers, uint64_t* bufferOffsets,uint64_t* strides,size_t bufferCount) override;


            VkCommandBuffer vulkanCommandBufferHandle();
            VkCommandPool vulkanCommandPoolHandle();

        protected:
            VkCommandBuffer _commandBuffer = nullptr;
            VkCommandPool _pool = nullptr;
            GPUQueue::QueueType _type = GPUQueue::QueueType::GRAPHICS;
#ifdef SLAG_DEBUG
            DescriptorPool* _boundDescriptorPool = nullptr;
#endif
        };
    } // vulkan
} // slag

#endif //SLAG_IVULKANCOMMANDBUFFER_H
