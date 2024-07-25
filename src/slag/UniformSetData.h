#ifndef SLAG_UNIFORMSETDATA_H
#define SLAG_UNIFORMSETDATA_H
#include "UniformSet.h"
#include "UniformBuffer.h"
#include "Texture.h"
#include "TextureSampler.h"
#include "UniformSetDataAllocator.h"
#include <vector>
namespace slag
{

    class UniformSetData
    {
    public:
        UniformSetData(UniformSet* provideFor, UniformSetDataAllocator* allocator);
        void setUniform(uint32_t index, BufferWriteData& writeData);
        void setTexture(uint32_t index, Texture* texture, TextureSampler* sampler,Texture::Layout layout);
        void setImage(uint32_t index, Texture* texture,Texture::Layout layout);
        UniformSet* providingFor()const;
        const void* lowLevelHandle()const;
    private:
        UniformSet* _provideFor = nullptr;
        void* _lowLevelHandle = nullptr;

    };

} // slag

#endif //SLAG_UNIFORMSETDATA_H
