#ifndef SLAG_DX12SHADERPIPELINE_H
#define SLAG_DX12SHADERPIPELINE_H
#include "../../ShaderPipeline.h"
#include <directx/d3d12shader.h>
#include "DX12DescriptorGroup.h"
#include "../../Resources/Resource.h"
namespace slag
{
    namespace dx
    {

        class DX12ShaderPipeline: public ShaderPipeline, resources::Resource
        {
        public:
            DX12ShaderPipeline(ShaderModule* modules, size_t moduleCount, DescriptorGroup** descriptorGroups, size_t descriptorGroupCount, const ShaderProperties& properties, VertexDescription* vertexDescription, FrameBufferDescription& frameBufferDescription, bool destroyImmediately);
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
        };

    } // dx
} // slag

#endif //SLAG_DX12SHADERPIPELINE_H
