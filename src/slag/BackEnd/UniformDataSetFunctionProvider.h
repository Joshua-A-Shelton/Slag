#ifndef SLAG_UNIFORMDATASETFUNCTIONPROVIDER_H
#define SLAG_UNIFORMDATASETFUNCTIONPROVIDER_H
#include "../UniformSet.h"
#include "../UniformSetDataAllocator.h"
#include "../UniformBuffer.h"
#include "../Texture.h"
#include "../TextureSampler.h"

namespace slag
{
    class UniformDataSetFunctionProvider
    {
    public:
        virtual ~UniformDataSetFunctionProvider()=default;
        virtual void initializeLowLevelHandle(void** lowLevelHandle, UniformSet* set, UniformSetDataAllocator* frame)=0;
        virtual void setUniformBufferData(void* lowLevelHandle, uint32_t uniformIndex,BufferWriteData* writtenData)=0;
        virtual void setTexture(void* lowLevelHandle, uint32_t uniformIndex, Texture* texture, TextureSampler* sampler, Texture::Layout layout)=0;
    };
}
#endif //SLAG_UNIFORMDATASETFUNCTIONPROVIDER_H
