#ifndef SLAG_DX12SEMAPHORE_H
#define SLAG_DX12SEMAPHORE_H
#include <slag/Slag.h>

#include <d3d12.h>

namespace slag
{
    namespace dx12
    {
        class DX12Semaphore: public Semaphore
        {
        public:
            DX12Semaphore(uint64_t initialValue);
            virtual ~DX12Semaphore()override;
            DX12Semaphore(const DX12Semaphore&) = delete;
            DX12Semaphore& operator=(const DX12Semaphore&) = delete;
            DX12Semaphore(DX12Semaphore&& from);
            DX12Semaphore& operator=(DX12Semaphore&& from);
            virtual uint64_t value()override;
            virtual void signal(uint64_t value)override;
            virtual void waitForValue(uint64_t value)override;
            static void waitFor(SemaphoreValue* values, size_t count);
            ID3D12Fence* dx12Handle()const;
        private:
            ID3D12Fence* _fence = nullptr;
        };
    } // dx12
} // slag

#endif //SLAG_DX12SEMAPHORE_H
