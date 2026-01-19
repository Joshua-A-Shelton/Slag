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
        const VertexInputAttribute& vertexInput(uint32_t index);
        uint32_t descriptorGroupsCount();
        const DescriptorBindingGroup& descriptorGroup(uint32_t index);
        uint32_t uniformBufferLayoutCount();
        const BufferDescriptorBindingLayout& uniformBufferLayout(uint32_t index);
        uint32_t storageBufferLayoutCount();
        const BufferDescriptorBindingLayout& storageBufferLayout(uint32_t index);
        uint32_t texelBufferDescriptionCount();
        const TexelBufferDescriptorBinding& texelBufferDescription(uint32_t index);
        const BufferLayout& pushConstantLayout();
        uint32_t xComputeThreads();
        uint32_t yComputeThreads();
        uint32_t zComputeThreads();

    private:
        void build(ShaderCode** stagePointers, uint32_t stageCount);
        ShaderStageFlags _stages = static_cast<ShaderStageFlags>(0);
        std::vector<VertexInputAttribute> _vertexInputs;
        std::vector<DescriptorBindingGroup> _bindingGroups;
        std::vector<BufferDescriptorBindingLayout> _uniformBufferLayouts;
        std::vector<BufferDescriptorBindingLayout> _storageBufferLayouts;
        std::vector<TexelBufferDescriptorBinding> _texelBufferDescriptions;
        BufferLayout _pushConstantLayout;
        uint32_t _xComputeThreads = 0;
        uint32_t _yComputeThreads = 0;
        uint32_t _zComputeThreads = 0;

    };

} // slag

#endif //SLAG_SHADERPIPELINEMETADATA_H