#ifndef SLAG_VULKANUNIFORMDATASETFUNCTIONPROVIDER_H
#define SLAG_VULKANUNIFORMDATASETFUNCTIONPROVIDER_H
#include "../UniformDataSetFunctionProvider.h"
#include "../../UniformSet.h"

namespace slag
{
    namespace vulkan
    {

        class VulkanUniformDataSetFunctionProvider: public UniformDataSetFunctionProvider
        {
        public:
            ~VulkanUniformDataSetFunctionProvider()override=default;
            void initializeLowLevelHandle(void** lowLevelHandle, UniformSet* set, UniformSetDataAllocator* allocator)override;
            void setUniformBufferData(void* lowLevelHandle, uint32_t uniformIndex,BufferWriteData* writtenData)override;
            void setTexture(void* lowLevelHandle, uint32_t uniformIndex, Texture* texture, TextureSampler* sampler, Texture::Layout layout)override;
            void setImage(void* lowLevelHandle, uint32_t uniformIndex, Texture* texture, Texture::Layout layout)override;
        };

    } // slag
} // vulkan

#endif //SLAG_VULKANUNIFORMDATASETFUNCTIONPROVIDER_H
