#ifndef SLAG_COMMANDBUFFER_H
#define SLAG_COMMANDBUFFER_H
#include "GPUMemoryBarriers.h"


namespace slag
{
    class CommandBuffer
    {
    public:
        virtual ~CommandBuffer()=default;
        virtual void insertMemoryBarrier(const GPUMemoryBarrier& barrier, PipelineStageFlags source, PipelineStageFlags destination)=0;
        virtual void insertMemoryBarriers(const GPUMemoryBarrier* barriers, size_t count, PipelineStageFlags source, PipelineStageFlags destination)=0;
        virtual void insertImageBarrier(const ImageMemoryBarrier& barrier, PipelineStageFlags source, PipelineStageFlags destination)=0;
        virtual void insertImageBarriers(const ImageMemoryBarrier* barriers, size_t count, PipelineStageFlags source, PipelineStageFlags destination)=0;
        virtual void insertBufferBarrier(const BufferMemoryBarrier& barrier, PipelineStageFlags source, PipelineStageFlags destination)=0;
        virtual void insertBufferBarriers(const BufferMemoryBarrier* barriers, size_t count, PipelineStageFlags source, PipelineStageFlags destination)=0;
        virtual void insertBarriers(const GPUMemoryBarrier* memoryBarriers, size_t memoryBarrierCount, const ImageMemoryBarrier* imageBarriers, size_t imageBarrierCount, const BufferMemoryBarrier* bufferBarriers, size_t bufferBarrierCount, PipelineStageFlags source, PipelineStageFlags destination)=0;

    };
}
#endif //SLAG_COMMANDBUFFER_H
