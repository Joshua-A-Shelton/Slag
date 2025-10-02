#ifndef SLAG_DX12SHADERPIPELINE_H
#define SLAG_DX12SHADERPIPELINE_H

#include <unordered_map>
#include <slag/Slag.h>

#include "D3D12MemAlloc.h"
#include "DX12DescriptorGroup.h"

namespace slag
{
    namespace dx12
    {
        class DX12ShaderPipeline: public ShaderPipeline
        {
        public:
            DX12ShaderPipeline(ShaderCode** shaders, size_t shaderCount, ShaderProperties& properties, VertexDescription& vertexDescription, FrameBufferDescription& framebufferDescription,std::string(*rename)(const std::string&,uint32_t,Descriptor::Type,Descriptor::Dimension,uint32_t, uint32_t,void*), void* renameData);
            DX12ShaderPipeline(const ShaderCode& computeShader,std::string(*rename)(const std::string&,uint32_t,Descriptor::Type,Descriptor::Dimension,uint32_t, uint32_t,void*), void* renameData);
            virtual ~DX12ShaderPipeline()override;
            DX12ShaderPipeline(const DX12ShaderPipeline&)=delete;
            DX12ShaderPipeline& operator=(const DX12ShaderPipeline&)=delete;
            DX12ShaderPipeline(DX12ShaderPipeline&& from);
            DX12ShaderPipeline& operator=(DX12ShaderPipeline&& from);

            ///What kind of shader pipeline this is
            virtual PipelineType pipelineType()override;
            ///Number of descriptor groups this shader has
            virtual uint32_t descriptorGroupCount()override;
            ///Retrieve descriptor group at index
            virtual DescriptorGroup* descriptorGroup(uint32_t index)override;
            ///Retrieve descriptor group at index
            virtual DescriptorGroup* operator[](uint32_t index)override;
            /**
             * Retrieve the layout of a buffer type descriptor
             * @param descriptorGroup the descriptor group index
             * @param descriptorBinding the binding of the buffer
             * @return Layout of a buffer descriptor (Uniform or Storage), or null if the index isn't a buffer type descriptor
             */
            virtual BufferLayout* bufferLayout(uint32_t descriptorGroup,uint32_t descriptorBinding)override;

            /**
             * Retrieve the description of a texel buffer type descriptor
             * @param descriptorGroup the descriptor group index
             * @param descriptorBinding the binding of the buffer
             * @return Layout of a texel buffer (Uniform or Storage), or null if the index isn't a texel buffer type descriptor
             */
            virtual TexelBufferDescription* texelBufferDescription(uint32_t descriptorGroup, uint32_t descriptorBinding)override;

            ///Number of compute threads in the x dimension (0 for graphics pipelines)
            virtual uint32_t xComputeThreads()override;
            ///Number of compute threads in the y dimension (0 for graphics pipelines)
            virtual uint32_t yComputeThreads()override;
            ///Number of compute threads in the z dimension (0 for graphics pipelines)
            virtual uint32_t zComputeThreads()override;
        private:
            void move(DX12ShaderPipeline& from);
            void spirvConstruct(ShaderCode** shaders, size_t shaderCount, ShaderProperties& properties, VertexDescription& vertexDescription, FrameBufferDescription& framebufferDescription, std::string(*rename)(const std::string&,uint32_t,Descriptor::Type,Descriptor::Dimension,uint32_t, uint32_t,void*), void* renameData);
            void dxilConstruct(ShaderCode** shaders, size_t shaderCount, ShaderProperties& properties, VertexDescription& vertexDescription, FrameBufferDescription& framebufferDescription, std::string(*rename)(const std::string&,uint32_t,Descriptor::Type,Descriptor::Dimension,uint32_t, uint32_t,void*), void* renameData);
            ID3D12PipelineState* _pipelineState = nullptr;
            PipelineType _pipelineType = PipelineType::GRAPHICS;
            std::vector<DX12DescriptorGroup> _descriptorGroups;
            std::unordered_map<uint32_t,std::unordered_map<uint32_t,BufferLayout>> _bufferLayouts;
            std::unordered_map<uint32_t,std::unordered_map<uint32_t,TexelBufferDescription>> _texelBufferDescriptions;
            uint32_t _xthreads = 0;
            uint32_t _ythreads = 0;
            uint32_t _zthreads = 0;

        };
    } // dx12
} // slag

#endif //SLAG_DX12SHADERPIPELINE_H
