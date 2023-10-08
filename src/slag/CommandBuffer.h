#ifndef SLAG_COMMANDBUFFER_H
#define SLAG_COMMANDBUFFER_H

#include <cstdlib>
#include "GPUMemoryBarriers.h"


namespace slag
{
    class CommandBuffer
    {
    public:
        enum Level
        {
            PRIMARY,
            SECONDARY
        };
        virtual ~CommandBuffer()=default;
        virtual Level level()=0;
        virtual void insertMemoryBarrier(const GPUMemoryBarrier& barrier, PipelineStageFlags source, PipelineStageFlags destination)=0;
        virtual void insertMemoryBarriers(const GPUMemoryBarrier* barriers, size_t count, PipelineStageFlags source, PipelineStageFlags destination)=0;
        virtual void insertImageBarrier(const ImageMemoryBarrier& barrier, PipelineStageFlags source, PipelineStageFlags destination)=0;
        virtual void insertImageBarriers(const ImageMemoryBarrier* barriers, size_t count, PipelineStageFlags source, PipelineStageFlags destination)=0;
        virtual void insertBufferBarrier(const BufferMemoryBarrier& barrier, PipelineStageFlags source, PipelineStageFlags destination)=0;
        virtual void insertBufferBarriers(const BufferMemoryBarrier* barriers, size_t count, PipelineStageFlags source, PipelineStageFlags destination)=0;
        virtual void insertBarriers(const GPUMemoryBarrier* memoryBarriers, size_t memoryBarrierCount, const ImageMemoryBarrier* imageBarriers, size_t imageBarrierCount, const BufferMemoryBarrier* bufferBarriers, size_t bufferBarrierCount, PipelineStageFlags source, PipelineStageFlags destination)=0;
        virtual void executeSecondaryCommands(CommandBuffer* subBuffer)=0;

    };
}
#endif //SLAG_COMMANDBUFFER_H
