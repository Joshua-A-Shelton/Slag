#include "DescriptorPool.h"
#include "BackEnd/BackEndLib.h"
namespace slag
{
    DescriptorBundle DescriptorPool::makeBundle(DescriptorGroup* forGroup)
    {
        DescriptorBundle bundle;
        bundle._handle = makeBundleLowLevelHandle(forGroup);
        return bundle;
    }

    DescriptorPool* DescriptorPool::newDescriptorPool(uint32_t samplers, uint32_t sampledTextures, uint32_t samplerAndTextureCombined,
                                      uint32_t storageTextures, uint32_t uniformTexelBuffers,
                                      uint32_t storageTexelBuffers, uint32_t uniformBuffers, uint32_t storageBuffers,
                                      uint32_t inputAttachments, uint32_t accelerationStructures)
    {
        return lib::BackEndLib::get()->newDescriptorPool(samplers,sampledTextures,samplerAndTextureCombined,storageTextures,uniformTexelBuffers,storageTexelBuffers,uniformBuffers,storageBuffers,inputAttachments,accelerationStructures);
    }
} // slag