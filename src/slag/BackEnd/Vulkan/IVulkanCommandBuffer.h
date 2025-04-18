#ifndef SLAG_IVULKANCOMMANDBUFFER_H
#define SLAG_IVULKANCOMMANDBUFFER_H
#include "../../CommandBuffer.h"
#include "slag/Rectangle.h"
#include "VulkanBuffer.h"
#include <vulkan/vulkan.h>
namespace slag
{
    namespace vulkan
    {
        class VulkanTexture;
        class IVulkanCommandBuffer: public CommandBuffer
        {
        public:
            virtual ~IVulkanCommandBuffer()=default;
            void end()override;

            GpuQueue::QueueType commandType()override;


            void insertBarriers(ImageBarrier* imageBarriers, size_t imageBarrierCount, BufferBarrier* bufferBarriers, size_t bufferBarrierCount, GPUMemoryBarrier* memoryBarriers, size_t memoryBarrierCount)override;
            void insertBarrier(const ImageBarrier& barrier)override;
            void insertBarrier(const BufferBarrier& barrier)override;
            void insertBarrier(const GPUMemoryBarrier& barrier)override;
            void clearColorImage(Texture* texture, ClearColor color, Texture::Layout currentLayout, Texture::Layout endingLayout, PipelineStages syncBefore, PipelineStages syncAfter)override;
            void updateMipChain(Texture* texture, uint32_t sourceMipLevel, Texture::Layout sourceLayout, Texture::Layout endingSourceLayout, Texture::Layout destinationLayout, Texture::Layout endingDestinationLayout, PipelineStages syncBefore, PipelineStages syncAfter)override;
            void copyBuffer(Buffer* source, size_t sourceOffset, size_t length, Buffer* destination, size_t destinationOffset)override;
            void copyImageToBuffer(Texture* texture,Texture::Layout layout, uint32_t baseLayer, uint32_t layerCount, uint32_t mip,Buffer* buffer, size_t bufferOffset)override;
            void copyBufferToImage(Buffer* source, size_t sourceOffset, Texture* destination, Texture::Layout destinationLayout, size_t layer, size_t mipLevel)override;
            void blit(Texture* source,Texture::Layout sourceLayout,uint32_t sourceLayer, uint32_t sourceMip,Rectangle sourceArea, Texture* destination, Texture::Layout destinationLayout,uint32_t destinationLayer, uint32_t destinationMip,Rectangle destinationArea,Sampler::Filter filter)override;

            void setViewPort(float x, float y, float width, float height, float minDepth,float maxDepth)override;
            void setScissors(Rectangle rectangle)override;
            void setBlendConstants(float r, float g, float b, float a)override;
            void setStencilReference(uint32_t reference)override;

            void beginQuery(QueryPool* queryPool, uint32_t query, bool precise)override;
            void beginRendering(Attachment* colorAttachments, size_t colorAttachmentCount,Attachment* depthAttachment,Rectangle bounds)override;
            void bindGraphicsDescriptorBundle(ShaderPipeline* shader, uint32_t index, DescriptorBundle& bundle)override;
            void bindComputeDescriptorBundle(ShaderPipeline* shader, uint32_t index, DescriptorBundle& bundle)override;
            void bindIndexBuffer(Buffer* buffer,Buffer::IndexSize indexSize, size_t offset)override;
            void bindGraphicsShader(ShaderPipeline* shader)override;
            void bindComputeShader(ShaderPipeline* shader)override;
            void bindVertexBuffers(uint32_t firstBinding, Buffer** buffers, size_t* offsets, size_t* sizes, size_t* strides, size_t bindingCount)override;
            //void clearAttachments(Attachment* attachments, size_t attachmentCount)override;
            void clearDepthStencilImage(Texture* texture, ClearDepthStencil clear, Texture::Layout currentLayout, Texture::Layout endingLayout, PipelineStages syncBefore, PipelineStages syncAfter)override;
            //void copyImageToImage()override;
            void copyQueryPoolResults(QueryPool* queryPool, uint32_t firstQuery, uint32_t queryCount, Buffer* destination, size_t offset, size_t stride,QueryPool::ResultFlag flags)override;
            void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)override;
            void dispatchBase(uint32_t baseGroupX,uint32_t baseGroupY, uint32_t baseGroupZ,uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)override;
            void dispatchIndirect(Buffer* buffer, size_t offset)override;
            void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)override;
            void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)override;
            void drawIndexedIndirect(Buffer* buffer, size_t offset, uint32_t drawCount, uint32_t stride)override;
            void drawIndexedIndirectCount(Buffer* buffer, size_t offset, Buffer* countBuffer, size_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride)override;
            void drawIndirect(Buffer* buffer, size_t offset, uint32_t drawCount, uint32_t stride)override;
            void drawIndirectCount(Buffer* buffer, size_t offset, Buffer* countBuffer, size_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride)override;
            void endQuery(QueryPool* pool, uint32_t query)override;
            void endRendering()override;
            //void executeCommands(ICommandBuffer** commands, size_t commandsCount)override;
            void fillBuffer(Buffer* buffer, size_t offset, size_t length, uint32_t data)override;
            void pushConstants(ShaderPipeline* shader, ShaderStages stageFlags, uint32_t offset, uint32_t size, const void* data)override;
            void resetQueryPool(QueryPool* pool, uint32_t firstQuery, uint32_t queryCount)override;
            //void resolve(Texture* source,Texture::Layout sourceLayout,uint32_t sourceLayer, uint32_t sourceMip,Rectangle sourceArea, Texture* destination, Texture::Layout destinationLayout,uint32_t destinationLayer, uint32_t destinationMip,Rectangle destinationArea)override;

            //vulkan only operations
            VkCommandBuffer underlyingCommandBuffer();

        protected:
            void move(IVulkanCommandBuffer& from);
            VkCommandBuffer _buffer = nullptr;
            VkCommandPool _pool = nullptr;
            uint32_t _family = 0;
        };

    } // vulkan
} // slag

#endif //SLAG_IVULKANCOMMANDBUFFER_H
