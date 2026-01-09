#ifndef SLAG_SAMPLERDESCRIPTORMEMORY_H
#define SLAG_SAMPLERDESCRIPTORMEMORY_H
#include <cstdint>
#include <slag/core/DescriptorMemory.h>
namespace slag
{
    class Sampler;

    class SamplerDescriptorMemory: public DescriptorMemory
    {
    public:
        virtual ~SamplerDescriptorMemory()override = default;
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
        virtual DescriptorMemory::Type type()final;
        static SamplerDescriptorMemory* newSamplerDescriptorMemory(uint64_t descriptorCount);
    };
} // slag

#endif //SLAG_SAMPLERDESCRIPTORMEMORY_H
