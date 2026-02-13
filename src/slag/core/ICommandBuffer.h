#ifndef SLAG_ICOMMANDBUFFER_H
#define SLAG_ICOMMANDBUFFER_H
#include <cstdint>

namespace slag
{
    class Buffer;
    class Texture;
    class TextureBufferMapping;
    ///Structure that contains the data for indirect draw calls
    struct IndirectDrawCommand
    {
        ///Number of vertices per instance
        uint32_t vertexCount = 0;
        ///Number of instances
        uint32_t instanceCount = 1;
        ///Offset into bound vertex buffer to start drawing from
        uint32_t firstVertex = 0;
        ///First instance ID (used in shaders)
        uint32_t firstInstance = 0;
    };

    ///Structure that contains the data for indirect indexed draw calls
    struct IndirectDrawIndexedCommand
    {
        ///indexCount Number of indexes per instance
        uint32_t indexCount = 0;
        ///Number of instances
        uint32_t instanceCount = 1;
        ///Offset into bound buffer to start drawing from
        uint32_t firstIndex = 0;
        ///Offset into bound vertex buffer to start drawing from
        int32_t vertexOffset = 0;
        ///First instance ID (used in shaders)
        uint32_t firstInstance = 0;
    };

    ///Structure that contains the data for indirect dispatch calls
    struct IndirectDispatchCommand
    {
        ///Local workgroups to dispatch in the X dimension
        uint32_t groupCountX = 1;
        ///Local workgroups to dispatch in the Y dimension
        uint32_t groupCountY = 1;
        ///Local workgroups to dispatch in the Z dimension
        uint32_t groupCountZ = 1;
    };

    class ICommandBuffer
    {
    public:
        virtual ~ICommandBuffer()=default;
        ///Begin recording commands
        virtual void begin();
        ///End recording commands
        virtual void end();
        /**
         * [TRANSFER] Copy data from one buffer to another
         * @param source Buffer to copy from
         * @param sourceOffset Byte offset to start the copy from
         * @param destination Buffer to copy to
         * @param destinationOffset Byte offset to copy the data to
         * @param length Number of bytes to copy
         */
        virtual void copyBufferToBuffer(Buffer* source, uint64_t sourceOffset, Buffer* destination, uint64_t destinationOffset, uint64_t length)=0;
        /**
         * [TRANSFER] Copy data from a texture to a buffer
         * @param source Texture to copy from
         * @param destination Buffer to copy to
         * @param copyData Array of structures containing the parameters of the copy operation
         * @param mappingCount Number of items in copyData array
         */
        virtual void copyTextureToBuffer(Texture* source, Buffer* destination, TextureBufferMapping* copyData, uint32_t mappingCount)=0;
        /**
         * [TRANSFER] Copy data from a buffer to a texture
         * @param source Buffer to copy data from
         * @param destination Texture to copy data to
         * @param copyData Array of structures containing the parameters of the copy operation
         * @param mappingCount Number of items in copyData array
         */
        virtual void copyBufferToTexture(Buffer* source, Texture* destination, TextureBufferMapping* copyData, uint32_t mappingCount)=0;
    };
} // slag

#endif //SLAG_ICOMMANDBUFFER_H
