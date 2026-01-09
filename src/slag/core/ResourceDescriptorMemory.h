#ifndef SLAG_RESOURCEDESCRIPTORMEMORY_H
#define SLAG_RESOURCEDESCRIPTORMEMORY_H
#include <slag/core/DescriptorMemory.h>
#include <slag/core/Texture.h>
#include <slag/core/Buffer.h>
namespace slag
{
    class ResourceDescriptorMemory: public DescriptorMemory
    {
    public:
        virtual ~ResourceDescriptorMemory()override=default;

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
         * Create a descriptor that points to a given uniform texel buffer. Buffer is treated as an array of objects of type 'format'
         * @param memoryLocation Location in the memory the new descriptor will be placed
         * @param buffer Buffer containing texel data
         * @param format Texel format the buffer contains
         * @param startIndex Index of first element
         * @param elementCount Number of elements to bind
         */
        virtual void setUniformTexelBuffer(uint64_t memoryLocation, Buffer* buffer, Pixels::Format format, uint64_t startIndex, uint64_t elementCount)=0;
        /**
         * Create a descriptor that points to a given storage texel buffer. Buffer is treated as an array of objects of type 'format'
         * @param memoryLocation Location in the memory the new descriptor will be placed
         * @param buffer Buffer containing texel data
         * @param format Texel format the buffer contains
         * @param startIndex Index of first element
         * @param elementCount Number of elements to bind
         */
        virtual void setStorageTexelBuffer(uint64_t memoryLocation, Buffer* buffer, Pixels::Format format, uint64_t startIndex, uint64_t elementCount)=0;
        /**
         * Create a descriptor that points to a given uniform buffer. Buffer is treated as an array of objects of of size 'dataStride'
         * @param memoryLocation Location in the memory the new descriptor will be placed
         * @param buffer Buffer the descriptor will reference
         * @param dataStride Size of an element in the buffer
         * @param startIndex Index of first element
         * @param elementCount Number of elements to bind
         */
        virtual void setUniformBuffer(uint64_t memoryLocation, Buffer* buffer, uint64_t dataStride, uint64_t startIndex, uint64_t elementCount)=0;
        /**
         * Create a descriptor that points to a given storage buffer. Buffer is treated as an array of objects of of size 'dataStride'
         * @param memoryLocation Location in the memory the new descriptor will be placed
         * @param buffer Buffer the descriptor will reference
         * @param dataStride Size of an element in the buffer
         * @param startIndex Index of first element
         * @param elementCount Number of elements to bind
         */
        virtual void setStorageBuffer(uint64_t memoryLocation, Buffer* buffer, uint64_t dataStride, uint64_t startIndex, uint64_t elementCount)=0;
        /**
         * Create a new resource descriptor memory block
         * @param descriptorCount number of descriptors this buffer can contain
         * @return
         */

        virtual DescriptorMemory::Type type() override;
        static ResourceDescriptorMemory* newResourceDescriptorMemory(uint64_t descriptorCount);
    };
} // slag

#endif //SLAG_RESOURCEDESCRIPTORMEMORY_H
