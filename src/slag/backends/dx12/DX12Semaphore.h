#ifndef SLAG_DX12SEMAPHORE_H
#define SLAG_DX12SEMAPHORE_H
#include <slag/Slag.h>
#include <d3d12.h>
namespace slag
{
    namespace dx12
    {
        class DX12GraphicsCard;
        class DX12Semaphore: public Semaphore
        {
        public:
            DX12Semaphore(DX12GraphicsCard* graphicsCard, uint64_t initialValue);
            DX12Semaphore(DX12Semaphore&)=delete;
            DX12Semaphore& operator=(DX12Semaphore&)=delete;
            DX12Semaphore(DX12Semaphore&& from) noexcept;
            DX12Semaphore& operator=(DX12Semaphore&& from) noexcept;
            ~DX12Semaphore()override;
            uint64_t value()override;
            void signal(uint64_t value)override;
            void waitForValue(uint64_t value)override;
            [[nodiscard]] GraphicsCard* graphicsCard()const override;
        private:
            void move(DX12Semaphore& from);
            ID3D12Fence* _fence = nullptr;
            DX12GraphicsCard* _graphicsCard = nullptr;
        };
    } // dx12
} // slag

#endif //SLAG_DX12SEMAPHORE_H
