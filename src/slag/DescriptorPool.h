#ifndef SLAG_DESCRIPTORPOOL_H
#define SLAG_DESCRIPTORPOOL_H

#include <cstdint>
#include "DescriptorGroup.h"
#include "DescriptorBundle.h"

namespace slag
{

    struct DescriptorPoolPageInfo
    {
        uint32_t samplers=1000;
        uint32_t sampledTextures=1000;
        uint32_t combinedSamplerTextures=1000;
        uint32_t storageTextures=1000;
        uint32_t uniformTexelBuffers=1000;
        uint32_t storageTexelBuffers=1000;
        uint32_t uniformBuffers=1000;
        uint32_t storageBuffers=1000;
        uint32_t inputAttachments=1000;
        uint32_t accelerationStructures=0;
        uint32_t descriptorBundles=1000;
    };

    class DescriptorPool
    {
    public:
        virtual ~DescriptorPool()=default;
        virtual void reset()=0;
        DescriptorBundle makeBundle(DescriptorGroup* forGroup);

        static DescriptorPool* newDescriptorPool();
        static DescriptorPool* newDescriptorPool(const DescriptorPoolPageInfo& pageInfo);
    protected:
        virtual void* makeBundleLowLevelHandle(DescriptorGroup* forGroup)=0;
    };

} // slag

#endif //SLAG_DESCRIPTORPOOL_H
