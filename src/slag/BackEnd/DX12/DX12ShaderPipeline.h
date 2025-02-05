#ifndef SLAG_DX12SHADERPIPELINE_H
#define SLAG_DX12SHADERPIPELINE_H
#include "../../ShaderPipeline.h"
#include <directx/d3d12shader.h>
#include "DX12DescriptorGroup.h"
#include "../../Resources/Resource.h"
#include "spirv_reflect.h"

namespace slag
{
    namespace dx
    {

        class DX12ShaderPipeline: public ShaderPipeline, resources::Resource
        {
        public:
            DX12ShaderPipeline(ShaderModule* modules, size_t moduleCount, DescriptorGroup** descriptorGroups, size_t descriptorGroupCount, const ShaderProperties& properties, VertexDescription* vertexDescription, FrameBufferDescription& frameBufferDescription, bool destroyImmediately);
            DX12ShaderPipeline(ShaderModule** modules, size_t moduleCount, DescriptorGroup** descriptorGroups, size_t descriptorGroupCount, const ShaderProperties& properties, VertexDescription* vertexDescription, FrameBufferDescription& frameBufferDescription, bool destroyImmediately);

            ~DX12ShaderPipeline()override;

            size_t descriptorGroupCount()override;
            DescriptorGroup* descriptorGroup(size_t index)override;
            DescriptorGroup* operator[](size_t index)override;
            size_t pushConstantRangeCount()override;
            PushConstantRange pushConstantRange(size_t index)override;
        private:
            void move(DX12ShaderPipeline&& from);
            ID3D12PipelineState* _pipeline = nullptr;
            std::vector<DX12DescriptorGroup> _descriptorGroups;
            std::vector<PushConstantRange> _pushConstantRanges;

            struct stageDetails
            {
                SpvReflectShaderModule reflectModule;
                ShaderStages stageFlags;
                bool own = false;
                stageDetails(ShaderModule& module): stageFlags(module.stage())
                {
                    spvReflectCreateShaderModule(module.dataSize(),module.data(),&reflectModule);
                    own = true;
                }
                ~stageDetails()
                {
                    if(own)
                    {
                        spvReflectDestroyShaderModule(&reflectModule);
                    }
                }
                stageDetails(stageDetails&& from):stageFlags(from.stageFlags)
                {
                    std::swap(reflectModule,from.reflectModule);
                    std::swap(own,from.own);
                }
                stageDetails& operator=(stageDetails&& from)
                {
                    std::swap(reflectModule,from.reflectModule);
                    std::swap(stageFlags,from.stageFlags);
                    std::swap(own,from.own);
                    return *this;
                }
            };

            void constructPipeline(DescriptorGroup** descriptorGroups, const size_t descriptorGroupCount, const ShaderProperties& properties, VertexDescription* vertexDescription, const FrameBufferDescription& frameBufferDescription,D3D12_GRAPHICS_PIPELINE_STATE_DESC& shaderDescription,std::vector<stageDetails>& shaderStageData,size_t vertexStageIndex);
        };

    } // dx
} // slag

#endif //SLAG_DX12SHADERPIPELINE_H
