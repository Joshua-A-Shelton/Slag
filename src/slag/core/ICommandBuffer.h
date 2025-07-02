#ifndef SLAG_ICOMMANDBUFFER_H
#define SLAG_ICOMMANDBUFFER_H
#include "GPUQueue.h"
#include "GPUBarriers.h"
#include "Clear.h"

namespace slag
{
    ///Provides the set of universal command buffer commands
    class ICommandBuffer
    {
    public:
        virtual ~ICommandBuffer()=default;
        ///The type of commands this command buffer can execute
        virtual GPUQueue::QueueType commandType()=0;
        ///Start recording commands
        virtual void begin()=0;
        ///End recording commands
        virtual void end()=0;

#ifdef SLAG_DISCREET_TEXTURE_LAYOUTS
        /**
         * Insert GPU execution barriers that guarantee certain operations finish before continuing execution of the graphics queue
         * @param textureBarriers Barriers that ensure an operation on a texture finishes before the next operation on it can continue
         * @param textureBarrierCount Number of texture barriers
         * @param bufferBarriers Barriers that ensure an operation on a buffer finishes before the next operation on it can continue
         * @param bufferBarrierCount Number of buffer barriers
         * @param memoryBarriers Barriers that ensure operations in the queue finishes before additional operations can continue
         * @param memoryBarrierCount Number of memory barriers
         */
        virtual void insertBarriers(TextureBarrierDiscreet* textureBarriers, size_t textureBarrierCount, BufferBarrier* bufferBarriers, size_t bufferBarrierCount, GPUMemoryBarrier* memoryBarriers, size_t memoryBarrierCount)=0;
        ///Insert GPU execution barrier that ensures an operation on a texture finishes before the next operation on it can continue
        virtual void insertBarrier(const TextureBarrierDiscreet& barrier)=0;
#else
        /**
         * Insert GPU execution barriers that guarantee certain operations finish before continuing execution of the graphics queue
         * @param textureBarriers Barriers that ensure an operation on a texture finishes before the next operation on it can continue
         * @param textureBarrierCount Number of texture barriers
         * @param bufferBarriers Barriers that ensure an operation on a buffer finishes before the next operation on it can continue
         * @param bufferBarrierCount Number of buffer barriers
         * @param memoryBarriers Barriers that ensure operations in the queue finishes before additional operations can continue
         * @param memoryBarrierCount Number of memory barriers
         */
        virtual void insertBarriers(TextureBarrier* textureBarriers, size_t textureBarrierCount, BufferBarrier* bufferBarriers, size_t bufferBarrierCount, GlobalBarrier* memoryBarriers, size_t memoryBarrierCount)=0;
        ///Insert GPU execution barrier that ensures an operation on a texture finishes before the next operation on it can continue
        virtual void insertBarrier(const TextureBarrier& barrier)=0;
#endif
        ///Insert GPU execution barrier that ensures an operation on a buffer finishes before the next operation on it can continue
        virtual void insertBarrier(const BufferBarrier& barrier)=0;
        ///Insert GPU execution barrier that ensures an operations in the queue finishes before additional operations can continue
        virtual void insertBarrier(const GlobalBarrier& barrier)=0;

        /**
         * Clear a color texture
         * @param texture The texture to clear
         * @param color The color to clear it to
         */
        virtual void clearTexture(Texture* texture, ClearColor color)=0;

        /**
         * clear a depth/stencil texture
         * @param texture The texture to clear
         * @param depthStencil The value to clear it to
         */
        virtual void clearTexture(Texture* texture, ClearDepthStencilValue depthStencil)=0;

        /**
         *
         * @param texture Texture to update
         * @param sourceMipLevel Mip level to source higher levels from
         * @param layer Texture layer to update
         */
        virtual void updateMipChain(Texture* texture, uint32_t sourceMipLevel, uint32_t layer)=0;

        /**
         *
         * @param source Buffer to copy from
         * @param sourceOffset Offset in the buffer to start copying from
         * @param length Length of data in bytes to copy to the new buffer
         * @param destination Buffer to copy to
         * @param destinationOffset Offset in the buffer to start copying to
         */
        virtual void copyBufferToBuffer(Buffer* source, uint64_t sourceOffset, uint64_t length, Buffer* destination, uint64_t destinationOffset)=0;

        virtual void copyTextureToBuffer(Texture* source, Buffer* destination)=0;


    };
} // slag

#endif //SLAG_ICOMMANDBUFFER_H
