#include "DX12Semaphore.h"

#include "DX12GraphicsCard.h"

namespace slag
{
    namespace dx12
    {
        DX12Semaphore::DX12Semaphore(uint64_t initialValue)
        {
            DX12GraphicsCard::selected()->device()->CreateFence(initialValue,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&_fence));
        }

        DX12Semaphore::~DX12Semaphore()
        {
            if (_fence) _fence->Release();
        }

        DX12Semaphore::DX12Semaphore(DX12Semaphore&& from)
        {
            std::swap(_fence, from._fence);
        }

        DX12Semaphore& DX12Semaphore::operator=(DX12Semaphore&& from)
        {
            std::swap(_fence, from._fence);
            return *this;
        }

        uint64_t DX12Semaphore::value()
        {
            return _fence->GetCompletedValue();
        }

        void DX12Semaphore::signal(uint64_t value)
        {
            _fence->Signal(value);
        }

        void DX12Semaphore::waitForValue(uint64_t value)
        {
            HANDLE handle = CreateEvent(nullptr,FALSE,FALSE,nullptr);
            _fence->SetEventOnCompletion(value,handle);
            WaitForSingleObject(handle,UINT32_MAX);
            CloseHandle(handle);
        }

        void DX12Semaphore::waitFor(SemaphoreValue* values, size_t count)
        {
            std::vector<HANDLE> events(count);
            for(size_t i =0; i< count; i++)
            {
                events[i] = CreateEvent(nullptr,FALSE,FALSE,nullptr);
                auto& v = values[i];
                auto semaphore = static_cast<DX12Semaphore*>(v.semaphore);
                semaphore->dx12Handle()->SetEventOnCompletion(v.value, events[i]);
            }

            WaitForMultipleObjects(count,events.data(),true,UINT32_MAX);
            for(size_t i=0; i< count; i++)
            {
                CloseHandle(events[i]);
            }
        }

        ID3D12Fence* DX12Semaphore::dx12Handle() const
        {
            return _fence;
        }
    } // dx12
} // slag
