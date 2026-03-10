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
            void copyBufferToBuffer(Buffer* source, uint64_t sourceOffset, Buffer* destination, uint64_t destinationOffset, uint64_t length)override;
            void copyTextureToBuffer(Texture* source, Buffer* destination, TextureBufferMapping* copyData, uint32_t mappingCount)override;
            void copyBufferToTexture(Buffer* source, Texture* destination, TextureBufferMapping* copyData, uint32_t mappingCount)override;

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
