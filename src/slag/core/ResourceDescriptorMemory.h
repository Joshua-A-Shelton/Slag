#ifndef SLAG_DESCRIPTORMEMORY_H
#define SLAG_DESCRIPTORMEMORY_H
#include <cstdint>
#include <slag/core/Texture.h>
#include <slag/core/Buffer.h>

namespace slag
{
    ///Memory backing descriptors of non-sampler types (usually a special kind of buffer)
    class ResourceDescriptorMemory
    {
    public:
        virtual ~ResourceDescriptorMemory() = default;
        ///Size in bytes of memory
        virtual uint64_t size()=0;

        /**
         * Get the location for the next descriptor group closest to the given memory address. Some API's require descriptor sets to be aligned to a boundary, so this call is required
         * @param memoryLocation Location in memory to check for the next aligned descriptor group location for
         * @return
         */
        virtual uint64_t descriptorGroupOffset(uint64_t memoryLocation)=0;
        /**
         * Create a descriptor that points to given sampled texture
         * @param memoryLocation Location in the memory the new descriptor will be placed
         * @param texture Texture the descriptor will reference
         */
        virtual void setSampledTexture(uint64_t memoryLocation, Texture* texture)=0;
        /**
         * Create a descriptor that points to a given storage texture
         * @param memoryLocation  Location in the memory the new descriptor will be placed
         * @param texture Texture the descriptor will reference
         */
        virtual void setStorageTexture(uint64_t memoryLocation, Texture* texture)=0;
        /**
         * Create a descriptor that points to a given uniform texel buffer
         * @param memoryLocation Location in the memory the new descriptor will be placed
         * @param buffer Buffer containing texel data
         * @param format Texel format the buffer contains
         * @param offset position into the buffer to bind
         * @param length number of bytes of the buffer to bind
         */
        virtual void setUniformTexelBuffer(uint64_t memoryLocation, Buffer* buffer, Pixels::Format format, uint64_t offset, uint64_t length)=0;
        /**
         * Create a descriptor that points to a given storage texel buffer
         * @param memoryLocation Location in the memory the new descriptor will be placed
         * @param buffer Buffer containing texel data
         * @param format Texel format the buffer contains
         * @param offset position into the buffer to bind
         * @param length number of bytes of the buffer to bind
         */
        virtual void setStorageTexelBuffer(uint64_t memoryLocation, Buffer* buffer, Pixels::Format format, uint64_t offset, uint64_t length)=0;
        /**
         * Create a descriptor that points to a given uniform buffer
         * @param memoryLocation Location in the memory the new descriptor will be placed
         * @param buffer Buffer the descriptor will reference
        * @param offset position into the buffer to bind
         * @param length number of bytes of the buffer to bind
         */
        virtual void setUniformBuffer(uint64_t memoryLocation, Buffer* buffer, uint64_t offset, uint64_t length)=0;
        /**
         * Create a descriptor that points to a given storage buffer
         * @param memoryLocation Location in the memory the new descriptor will be placed
         * @param buffer Buffer the descriptor will reference
         * @param offset position into the buffer to bind
         * @param length number of bytes of the buffer to bind
         */
        virtual void setStorageBuffer(uint64_t memoryLocation, Buffer* buffer, uint64_t offset, uint64_t length)=0;
        /**
         * Create a new resource descriptor memory block
         * @param descriptorCount number of descriptors this buffer can contain
         * @return
         */
        static ResourceDescriptorMemory* newResourceDescriptorMemory(uint64_t descriptorCount);
    };
} // slag

#endif //SLAG_DESCRIPTORMEMORY_H
