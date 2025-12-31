#ifndef SLAG_SAMPLERDESCRIPTORMEMORY_H
#define SLAG_SAMPLERDESCRIPTORMEMORY_H
#include <cstdint>

namespace slag
{
    class Sampler;

    class SamplerDescriptorMemory
    {
    public:
        virtual ~SamplerDescriptorMemory() = default;
        ///Size in bytes of memory
        virtual uint64_t size()=0;
        /**
         * Get the location for the next descriptor group closest to the given memory address. Some API's require descriptor sets to be aligned to a boundary, so this call is required
         * @param memoryLocation Location in memory to check for the next aligned descriptor group location for
         * @return
         */
        virtual uint64_t descriptorGroupOffset(uint64_t memoryLocation)=0;
        /**
         * Create a descriptor that points to a given sampler
         * @param memoryLocation Location in the memory the new descriptor will be placed
         * @param sampler Sampler the descriptor will reference
         */
        virtual void setSampler(uint64_t memoryLocation, Sampler* sampler)=0;
        /**
         * Create a new sampler descriptor memory block
         * @param descriptorCount number of descriptors this buffer can contain
         * @return
         */
        static SamplerDescriptorMemory* newSamplerDescriptorMemory(uint64_t descriptorCount);
    };
} // slag

#endif //SLAG_SAMPLERDESCRIPTORMEMORY_H
