#ifndef SLAG_VULKANCOMMANDBUFFER_H
#define SLAG_VULKANCOMMANDBUFFER_H
#include "../../CommandBuffer.h"
#include "../Resource.h"
#include <vulkan/vulkan.h>
#include "VulkanDescriptorAllocator.h"

namespace slag
{
    namespace vulkan
    {
        class VulkanCommandBuffer: public CommandBuffer, Resource
        {
        private:
            VkCommandBuffer _cmdBuffer = nullptr;
            VkCommandPool _pool = nullptr;
            bool _primary = false;
            VkQueue _submissionQueue;
            void move(VulkanCommandBuffer&& from);
        public:
            VulkanCommandBuffer(VkCommandPool pool,bool primary, VkQueue submissionQueue, bool destroyImmediately);
            VulkanCommandBuffer(bool primary,VkQueue submissionQueue, uint32_t queueFamily, bool destroyImmediately);
            VulkanCommandBuffer(const VulkanCommandBuffer&)=delete;
            VulkanCommandBuffer& operator=(const VulkanCommandBuffer&)=delete;
            VulkanCommandBuffer(VulkanCommandBuffer&& from);
            VulkanCommandBuffer& operator=(VulkanCommandBuffer&& from);
            ~VulkanCommandBuffer()override;
            VkCommandBuffer& vulkanCommandBuffer();
            void* GPUID()override;

            void reset();
            void begin();
            void end();
            void submit(VkSemaphore* waitSemaphores, uint32_t waitCount, VkSemaphore* signalSemaphores, uint32_t signalCount, VkFence fence);
            Level level()override;
            void insertMemoryBarrier(const GPUMemoryBarrier& barrier, PipelineStageFlags source, PipelineStageFlags destination)override;
            void insertMemoryBarriers(const GPUMemoryBarrier* barriers, size_t count, PipelineStageFlags source, PipelineStageFlags destination)override;
            void insertImageBarrier(const ImageMemoryBarrier& barrier, PipelineStageFlags source, PipelineStageFlags destination)override;
            void insertImageBarriers(const ImageMemoryBarrier* barriers, size_t count, PipelineStageFlags source, PipelineStageFlags destination)override;
            void insertBufferBarrier(const BufferMemoryBarrier& barrier, PipelineStageFlags source, PipelineStageFlags destination)override;
            void insertBufferBarriers(const BufferMemoryBarrier* barriers, size_t count, PipelineStageFlags source, PipelineStageFlags destination)override;
            void insertBarriers(const GPUMemoryBarrier* memoryBarriers, size_t memoryBarrierCount, const ImageMemoryBarrier* imageBarriers, size_t imageBarrierCount, const BufferMemoryBarrier* bufferBarriers, size_t bufferBarrierCount, PipelineStageFlags source, PipelineStageFlags destination)override;
            void executeSecondaryCommands(CommandBuffer* subBuffer)override;
            void setTargetFramebuffer(Rectangle bounds,Attachment* colorAttachments, size_t colorCount)override;
            void setTargetFramebuffer(Rectangle bounds,Attachment* colorAttachments, size_t colorCount, Attachment depthAttachment)override;
            void endTargetFramebuffer();
            void setViewport(Rectangle bounds)override;
            void setScissors(Rectangle bounds)override;
            void bindVertexBuffer(Buffer* vertexBuffer)override;
            void bindIndexBuffer(Buffer* indexBuffer, GraphicsTypes::IndexType indexType)override;
            void bindShader(Shader* shader)override;
            void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)override;
            void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstVertex, int32_t vertexOffset, uint32_t firstInstance)override;
            void bindUniformData(Shader*, UniformSet* set, uint32_t uniformIndex, UniformBuffer* writeToBuffer, void* data, size_t dataSize)override;
            void bindUniformData(Shader* shader, UniformSet* set, uint32_t uniformIndex, UniformData writeLocation)override;

            //TODO: move this out from here. It's used here most frequently, (possibly exclusively), but has nothing to do with command buffers directly
            static VkPipelineStageFlags pipelineStageFromCrossPlatform(PipelineStageFlags flags);

        };
    } // slag
} // vulkan
#endif //SLAG_VULKANCOMMANDBUFFER_H