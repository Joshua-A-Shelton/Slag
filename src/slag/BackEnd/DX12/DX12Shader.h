#ifndef SLAG_DX12SHADER_H
#define SLAG_DX12SHADER_H
#include "../../Shader.h"
#include <directx/d3d12shader.h>
#include "DX12DescriptorGroup.h"
#include "../../Resources/Resource.h"
namespace slag
{
    namespace dx
    {

        class DX12Shader: public Shader, resources::Resource
        {
        public:
            DX12Shader(ShaderModule* modules, size_t moduleCount, DescriptorGroup** descriptorGroups, size_t descriptorGroupCount, const ShaderProperties& properties, VertexDescription* vertexDescription, FrameBufferDescription& frameBufferDescription, bool destroyImmediately);
            ~DX12Shader()override;

            size_t descriptorGroupCount()override;
            DescriptorGroup* descriptorGroup(size_t index)override;
            DescriptorGroup* operator[](size_t index)override;
            size_t pushConstantRangeCount()override;
            PushConstantRange pushConstantRange(size_t index)override;
        private:
            void move(DX12Shader&& from);
            std::vector<DX12DescriptorGroup> _descriptorGroups;
            std::vector<PushConstantRange> _pushConstantRanges;
        };

    } // dx
} // slag

#endif //SLAG_DX12SHADER_H
