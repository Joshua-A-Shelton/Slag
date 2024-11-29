#ifndef SLAG_DESCRIPTORPOOL_H
#define SLAG_DESCRIPTORPOOL_H

#include <cstdint>
#include "DescriptorGroup.h"
#include "DescriptorBundle.h"

namespace slag
{

    class DescriptorPool
    {
    public:
        virtual ~DescriptorPool()=default;
        virtual void reset()=0;
        DescriptorBundle makeBundle(DescriptorGroup* forGroup);

        static DescriptorPool* newDescriptorPool(uint32_t samplers,
                                                 uint32_t sampledTextures,
                                                 uint32_t samplerAndTextureCombined,
                                                 uint32_t storageTextures,
                                                 uint32_t uniformTexelBuffers,
                                                 uint32_t storageTexelBuffers,
                                                 uint32_t uniformBuffers,
                                                 uint32_t storageBuffers,
                                                 uint32_t inputAttachments,
                                                 uint32_t accelerationStructures);
    protected:
        virtual void* makeBundleLowLevelHandle(DescriptorGroup* forGroup)=0;
    };

} // slag

#endif //SLAG_DESCRIPTORPOOL_H
