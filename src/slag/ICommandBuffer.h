#ifndef SLAG_ICOMMANDBUFFER_H
#define SLAG_ICOMMANDBUFFER_H

#include "GpuQueue.h"
#include "Clear.h"
#include "GpuMemoryBarriers.h"
#include "Rectangle.h"
#include "Sampler.h"
#include "DescriptorGroup.h"

namespace slag
{
    class Texture;
    class Buffer;
    class ICommandBuffer
    {
    public:
        virtual GpuQueue::QueueType commandType()=0;
        virtual void begin()=0;
        virtual void end()=0;
        virtual void insertBarriers(ImageBarrier* imageBarriers, size_t imageBarrierCount, BufferBarrier* bufferBarriers, size_t bufferBarrierCount, GPUMemoryBarrier* memoryBarriers, size_t memoryBarrierCount)=0;
        virtual void clearColorImage(Texture* texture, ClearColor color, Texture::Layout currentLayout, Texture::Layout endingLayout, PipelineStages syncBefore, PipelineStages syncAfter)=0;
        virtual void updateMipChain(Texture* texture, uint32_t sourceMipLevel, Texture::Layout sourceLayout, Texture::Layout endingSourceLayout, Texture::Layout destinationLayout, Texture::Layout endingDestinationLayout, PipelineStages syncBefore, PipelineStages syncAfter)=0;
        virtual void copyBuffer(Buffer* source, size_t sourceOffset, size_t length, Buffer* destination, size_t destinationOffset)=0;
        virtual void copyImageToBuffer(Texture* texture,Texture::Layout layout, uint32_t baseLayer, uint32_t layerCount, uint32_t mip,Buffer* buffer, size_t bufferOffset)=0;
        virtual void copyBufferToImage(Buffer* source, size_t sourceOffset, Texture* destination, Texture::Layout destinationLayout, size_t layer, size_t mipLevel)=0;
        virtual void blit(Texture* source,Texture::Layout sourceLayout,uint32_t sourceLayer, uint32_t sourceMip,Rectangle sourceArea, Texture* destination, Texture::Layout destinationLayout,uint32_t destinationLayer, uint32_t destinationMip,Rectangle destinationArea,Sampler::Filter filter)=0;

    };
}
#endif //SLAG_ICOMMANDBUFFER_H
