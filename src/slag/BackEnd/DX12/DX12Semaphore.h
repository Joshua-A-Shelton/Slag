#ifndef SLAG_DX12SEMAPHORE_H
#define SLAG_DX12SEMAPHORE_H

#include "../../Semaphore.h"
#include "../../Resources/Resource.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

namespace slag
{
    namespace dx
    {

        class DX12Semaphore: public Semaphore, resources::Resource
        {
        public:
            DX12Semaphore(uint64_t initialValue, bool destroyImmediately = false);
            ~DX12Semaphore();
            DX12Semaphore(const DX12Semaphore&)=delete;
            DX12Semaphore& operator=(const DX12Semaphore&)=delete;
            DX12Semaphore(DX12Semaphore&& from);
            DX12Semaphore& operator=(DX12Semaphore&& from);

            void* gpuID()override;

            uint64_t value()override;
            void signal(uint64_t value)override;
            void waitForValue(uint64_t value)override;
            ID3D12Fence* fence();
            static void waitFor(SemaphoreValue* values, size_t count);
        private:
            ID3D12Fence* _fence = nullptr;
        };

    } // dx
} // slag

#endif //CRUCIBLEEDITOR_DX12SEMAPHORE_H
