#ifndef SLAG_IDX12COMMANDBUFFER_H
#define SLAG_IDX12COMMANDBUFFER_H
#include <slag/Slag.h>
#include <d3d12.h>

namespace slag
{
    namespace dx12
    {
        class DX12GraphicsCard;

        class IDX12CommandBuffer: public CommandBuffer
        {
        public:
            ~IDX12CommandBuffer()override = default;
            [[nodiscard]] GraphicsCard* graphicsCard() override;
            [[nodiscard]] QueueType type()const override;

            void begin() override;
            void end() override;
            void insertBarriers(GlobalBarrier* barriers, uint32_t barrierCount)override;
            void insertBarriers(BufferBarrier* barriers, uint32_t barrierCount)override;
            void insertBarriers(TextureBarrier* barriers, uint32_t barrierCount)override;
            void insertBarriers(
                GlobalBarrier* globalBarriers,
                uint32_t globalBarrierCount,
                BufferBarrier* bufferBarriers,
                uint32_t bufferBarrierCount,
                TextureBarrier* textureBarriers,
                uint32_t textureBarrierCount
                )override;
            void bindDescriptorHeaps(DescriptorHeap* resourceHeap, DescriptorHeap* samplerHeap)override;
            void setInputBindingTable(uint32_t byteOffset, uint32_t heapOffset)override;
            void copyBufferToBuffer(Buffer* source, uint64_t sourceOffset, Buffer* destination, uint64_t destinationOffset, uint64_t length)override;
            void copyTextureToBuffer(Texture* source, Buffer* destination, TextureBufferMapping* copyData, uint32_t mappingCount)override;
            void copyBufferToTexture(Buffer* source, Texture* destination, TextureBufferMapping* copyData, uint32_t mappingCount)override;
            void bindGraphicsPipeline(ShaderPipeline* pipeline)override;
            void beginRendering(Attachment* colorAttachments, uint32_t colorAttachmentCount,Attachment* depthAttachment, const Rectangle& bounds)override;
            void endRendering()override;
            void setViewPort(float x, float y, float width, float height, float minDepth, float maxDepth)override;
            void setScissors(const Rectangle& rect)override;
            void bindIndexBuffer(Buffer* buffer, IndexBufferType indexType, uint64_t offset)override;
            void bindVertexBuffers(uint32_t firstBinding, Buffer** buffers, uint64_t* bufferOffsets, uint64_t* strides, uint32_t bufferCount)override;
            void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)override;
            void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)override;

            [[nodiscard]] ID3D12GraphicsCommandList7* dx12Handle() const;

            protected:
            void IDXCBMove(IDX12CommandBuffer& from);
            ID3D12GraphicsCommandList7* _commandBuffer = nullptr;
            ID3D12CommandAllocator* _commandPool = nullptr;
            DX12GraphicsCard* _graphicsCard = nullptr;
            QueueType _queueType = QueueType::TRANSFER;
        };
    } // dx12
} // slag

#endif //SLAG_IDX12COMMANDBUFFER_H
