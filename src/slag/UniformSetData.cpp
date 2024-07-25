#include "UniformSetData.h"
#include "BackEnd/UniformDataSetHandler.h"
namespace slag
{

    UniformSetData::UniformSetData(UniformSet* provideFor, UniformSetDataAllocator* allocator)
    {
        _provideFor = provideFor;
        UniformDataSetHandler::dataProvider()->initializeLowLevelHandle(&_lowLevelHandle,provideFor,allocator);
    }

    void UniformSetData::setUniform(uint32_t index, BufferWriteData &writeData)
    {
        UniformDataSetHandler::dataProvider()->setUniformBufferData(_lowLevelHandle,index,&writeData);
    }

    void UniformSetData::setTexture(uint32_t index, Texture *texture, TextureSampler *sampler,Texture::Layout layout)
    {
        UniformDataSetHandler::dataProvider()->setTexture(_lowLevelHandle,index,texture,sampler,layout);
    }

    void UniformSetData::setImage(uint32_t index, Texture* texture, Texture::Layout layout)
    {
        UniformDataSetHandler::dataProvider()->setImage(_lowLevelHandle,index,texture,layout);
    }

    UniformSet *UniformSetData::providingFor() const
    {
        return _provideFor;
    }

    const void *UniformSetData::lowLevelHandle() const
    {
        return _lowLevelHandle;
    }
}