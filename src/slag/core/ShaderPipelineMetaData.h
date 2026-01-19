#ifndef SLAG_SHADERPIPELINEMETADATA_H
#define SLAG_SHADERPIPELINEMETADATA_H
#include "ShaderCode.h"

namespace slag
{


    class ShaderPipelineMetaData
    {
    public:
        ShaderPipelineMetaData(ShaderCode** stagePointers, uint32_t stageCount);
        ShaderPipelineMetaData(ShaderCode* stages, uint32_t stageCount);
        ShaderStageFlags stageFlags();
        uint32_t vertexInputCount();
        const ShaderVertexInputVariable& vertexInput(uint32_t index);
        uint32_t descriptorGroupsCount();
        const ShaderDescriptorBindingGroup& descriptorGroup(uint32_t index);
        uint32_t uniformBufferLayoutCount();
        const ShaderBufferLayout& uniformBufferLayout(uint32_t index);
        uint32_t storageBufferLayoutCount();
        const ShaderBufferLayout& storageBufferLayout(uint32_t index);
        uint32_t texelBufferDescriptionCount();
        const ShaderTexelBufferDescription& texelBufferDescription(uint32_t index);
        const BufferLayout& pushConstantLayout();
        uint32_t xComputeThreads();
        uint32_t yComputeThreads();
        uint32_t zComputeThreads();

    private:
        void build(ShaderCode** stagePointers, uint32_t stageCount);
        ShaderStageFlags _stages = static_cast<ShaderStageFlags>(0);
        std::vector<ShaderVertexInputVariable> _vertexInputs;
        std::vector<ShaderDescriptorBindingGroup> _bindingGroups;
        std::vector<ShaderBufferLayout> _uniformBufferLayouts;
        std::vector<ShaderBufferLayout> _storageBufferLayouts;
        std::vector<ShaderTexelBufferDescription> _texelBufferDescriptions;
        BufferLayout _pushConstantLayout;
        uint32_t _xComputeThreads = 0;
        uint32_t _yComputeThreads = 0;
        uint32_t _zComputeThreads = 0;

    };

} // slag

#endif //SLAG_SHADERPIPELINEMETADATA_H