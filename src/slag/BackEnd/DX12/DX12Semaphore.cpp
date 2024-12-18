#include "DX12Semaphore.h"
#include "DX12Lib.h"

namespace slag
{
    namespace dx
    {
        DX12Semaphore::DX12Semaphore(uint64_t initialValue, bool destroyImmediately): resources::Resource(destroyImmediately)
        {
            DX12Lib::card()->device()->CreateFence(initialValue,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&_fence));

            auto fence = _fence;
            _disposeFunction = [=]
            {
                fence->Release();
            };
        }

        DX12Semaphore::~DX12Semaphore()
        {
            if(_fence)
            {
                smartDestroy();
            }
        }

        DX12Semaphore::DX12Semaphore(DX12Semaphore&& from): resources::Resource(from._destroyImmediately)
        {
            resources::Resource::move(from);
            _fence = from._fence;
            from._fence = nullptr;

        }

        DX12Semaphore& DX12Semaphore::operator=(DX12Semaphore&& from)
        {
            resources::Resource::move(from);
            _fence = from._fence;
            from._fence = nullptr;
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

        ID3D12Fence* DX12Semaphore::fence()
        {
            return _fence;
        }

        void DX12Semaphore::waitFor(SemaphoreValue* values, size_t count)
        {
            std::vector<HANDLE> events(count);
            for(size_t i =0; i< count; i++)
            {
                events[i] = CreateEvent(nullptr,FALSE,FALSE,nullptr);
                auto& v = values[i];
                auto semaphore = static_cast<DX12Semaphore*>(v.semaphore);
                semaphore->fence()->SetEventOnCompletion(v.value, events[i]);
            }

            WaitForMultipleObjects(count,events.data(),true,UINT32_MAX);
            for(size_t i=0; i< count; i++)
            {
                CloseHandle(events[i]);
            }
        }

    } // dx
} // slag