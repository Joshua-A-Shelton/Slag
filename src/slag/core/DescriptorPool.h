#ifndef SLAG_DESCRIPTORPOOL_H
#define SLAG_DESCRIPTORPOOL_H
#include <cstdint>

#include "DescriptorBundle.h"

namespace slag
{
    class DescriptorGroup;

    ///Describes how a descriptor pool should grow, should the underlying API require it
    struct DescriptorPoolPageInfo
    {
        ///Number of sampler descriptors that can be created from the pool, may be ignored by underlying API
        uint32_t samplers=1000;
        ///Number of sampledTexture descriptors that can be created from the pool, may be ignored by underlying API
        uint32_t sampledTextures=1000;
        ///Number of storage texture descriptors that can be created from the pool, may be ignored by underlying API
        uint32_t storageTextures=1000;
        ///Number of uniform texel buffer descriptors that can be created from the pool, may be ignored by underlying API
        uint32_t uniformTexelBuffers=1000;
        ///Number of storage texel buffer descriptors that can be created from the pool, may be ignored by underlying API
        uint32_t storageTexelBuffers=1000;
        ///Number of uniform buffer descriptors that can be created from the pool, may be ignored by underlying API
        uint32_t uniformBuffers=1000;
        ///Number of storage buffer descriptors that can be created from the pool, may be ignored by underlying API
        uint32_t storageBuffers=1000;
        ///Number of input attachment descriptors that can be created from the pool, may be ignored by underlying API
        uint32_t inputAttachments=1000;
        ///Number of descriptor bundle descriptors that can be created from the pool, may be ignored by underlying API
        uint32_t accelerationStructures=0;
        ///Number of descriptor bundle descriptors that can be created from the pool, may be ignored by underlying API
        uint32_t descriptorBundles=1000;

    };
    ///Pool for which descriptors can be assigned via DescriptorBundles
    class DescriptorPool
    {
    public:
        virtual ~DescriptorPool()=default;
        virtual void reset()=0;
        DescriptorBundle makeBundle(DescriptorGroup* forGroup);

        static DescriptorPool* newDescriptorPool();
        static DescriptorPool* newDescriptorPool(const DescriptorPoolPageInfo& pageInfo);
    protected:
        virtual void setBundleLowLevelHandles(void** gpuHandle, void** cpuHandle, DescriptorGroup* forGroup) =0;
    };
} // slag

#endif //SLAG_DESCRIPTORPOOL_H
