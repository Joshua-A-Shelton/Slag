#ifndef SLAG_DX12SHADERPIPELINE_H
#define SLAG_DX12SHADERPIPELINE_H
#include <slag/Slag.h>
#include <d3d12.h>

namespace slag
{
    namespace dx12
    {
        class DX12GraphicsCard;

        class DX12ShaderPipeline: public ShaderPipeline
        {
        public:
            DX12ShaderPipeline(
               DX12GraphicsCard* graphicsCard,
               const VertexDescription& vertexDescription,
               const ShaderCode& vertexShader,
               const ShaderCode& fragmentShader,
               const PipelineState& pipelineState,
               const FramebufferDescription& framebufferDescription);

            DX12ShaderPipeline(
                DX12GraphicsCard* graphicsCard,
                ShaderCode* computeShader);

            DX12ShaderPipeline(const DX12ShaderPipeline&) = delete;
            DX12ShaderPipeline& operator=(const DX12ShaderPipeline&) = delete;
            DX12ShaderPipeline(DX12ShaderPipeline&& from) noexcept;
            DX12ShaderPipeline& operator=(DX12ShaderPipeline&& from) noexcept;
            ~DX12ShaderPipeline()override;

            ShaderPipelineType type()override;
            GraphicsCard* graphicsCard()override;

            ID3D12PipelineState* dx12Handle() const;
        private:
            void move(DX12ShaderPipeline& from);
            DX12GraphicsCard* _graphicsCard = nullptr;
            ID3D12PipelineState* _pipelineState = nullptr;
            ShaderPipelineType _type = ShaderPipelineType::GRAPHICS;
        };
    } // dx12
} // slag

#endif //SLAG_DX12SHADERPIPELINE_H