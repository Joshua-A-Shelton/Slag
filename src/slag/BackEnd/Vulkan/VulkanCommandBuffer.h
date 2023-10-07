#ifndef SLAG_VULKANCOMMANDBUFFER_H
#define SLAG_VULKANCOMMANDBUFFER_H
#include "../../CommandBuffer.h"
#include <vulkan/vulkan.h>
namespace slag
{
    namespace vulkan
    {
        class VulkanCommandBuffer: public CommandBuffer
        {
        private:
            VkCommandBuffer _cmdBuffer = nullptr;
            VkCommandPool _pool = nullptr;
            void move(VulkanCommandBuffer&& from);
        public:
            VulkanCommandBuffer(VkCommandPool pool);
            VulkanCommandBuffer(const VulkanCommandBuffer&)=delete;
            VulkanCommandBuffer& operator=(const VulkanCommandBuffer&)=delete;
            VulkanCommandBuffer(VulkanCommandBuffer&& from);
            VulkanCommandBuffer& operator=(VulkanCommandBuffer&& from);
            ~VulkanCommandBuffer()override;
            VkCommandBuffer& vulkanCommandBuffer();

            void reset();
            void begin();
            void end();
            void submit(VkSemaphore* waitSemaphores, uint32_t waitCount, VkSemaphore* signalSemaphores, uint32_t signalCount, VkQueue submitQueue, VkFence fence);

            void insertMemoryBarrier(const GPUMemoryBarrier& barrier, PipelineStageFlags source, PipelineStageFlags destination)override;
            void insertMemoryBarriers(const GPUMemoryBarrier* barriers, size_t count, PipelineStageFlags source, PipelineStageFlags destination)override;
            void insertImageBarrier(const ImageMemoryBarrier& barrier, PipelineStageFlags source, PipelineStageFlags destination)override;
            void insertImageBarriers(const ImageMemoryBarrier* barriers, size_t count, PipelineStageFlags source, PipelineStageFlags destination)override;
            void insertBufferBarrier(const BufferMemoryBarrier& barrier, PipelineStageFlags source, PipelineStageFlags destination)override;
            void insertBufferBarriers(const BufferMemoryBarrier* barriers, size_t count, PipelineStageFlags source, PipelineStageFlags destination)override;
            void insertBarriers(const GPUMemoryBarrier* memoryBarriers, size_t memoryBarrierCount, const ImageMemoryBarrier* imageBarriers, size_t imageBarrierCount, const BufferMemoryBarrier* bufferBarriers, size_t bufferBarrierCount, PipelineStageFlags source, PipelineStageFlags destination)override;

            //TODO: move this out from here. It's used here most frequently, (possibly exclusively), but has nothing to do with command buffers directly
            static VkPipelineStageFlags pipelineStageFromCrossPlatform(PipelineStageFlags flags);

        };
    } // slag
} // vulkan
#endif //SLAG_VULKANCOMMANDBUFFER_H