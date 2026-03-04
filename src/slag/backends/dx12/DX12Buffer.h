#ifndef SLAG_DX12BUFFER_H
#define SLAG_DX12BUFFER_H
#include <slag/Slag.h>
#include <D3D12MemAlloc.h>

namespace slag
{
    namespace dx12
    {
        class DX12GraphicsCard;
        const uint8_t DXBUFFER_MEMORY_BITS = 0b00000001;
        const uint8_t DXBUFFER_CPU_BITS    = 0b00000110;

        const uint8_t DXBUFFER_CPU_SHIFT = 1;

        class DX12Buffer: public Buffer
        {
        public:
            DX12Buffer(
                DX12GraphicsCard* card,
                uint64_t size,
                BufferCPUAccess cpuAccess,
                BufferMemoryType memoryType);
            ~DX12Buffer()override;
            DX12Buffer(const DX12Buffer&) = delete;
            DX12Buffer& operator=(const DX12Buffer&) = delete;
            DX12Buffer(DX12Buffer&& from) noexcept;
            DX12Buffer& operator=(DX12Buffer&& from) noexcept;

            [[nodiscard]] void* data()const override;
            [[nodiscard]] uint64_t deviceAddress()const override;
            [[nodiscard]] BufferMemoryType memoryType()const override;
            [[nodiscard]] BufferCPUAccess cpuAccess()const override;
            [[nodiscard]] uint64_t size()const override;
            [[nodiscard]] GraphicsCard* graphicsCard() const override;
            [[nodiscard]] void* userData() override;
            void setUserData(void* userData) override;

            [[nodiscard]] ID3D12Resource* dx12Handle()const;

        private:
            void move(DX12Buffer& from);
            uint64_t _size = 0;
            ID3D12Resource* _buffer = nullptr;
            D3D12MA::Allocation* _allocation = nullptr;
            DX12GraphicsCard* _graphicsCard = nullptr;
            void* _cpuHandle = nullptr;
            void* _userData = nullptr;
            uint8_t _dataBits = 0;
        };
    } // dx12
} // slag

#endif //SLAG_DX12BUFFER_H
