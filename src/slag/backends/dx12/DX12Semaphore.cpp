#include "DX12Semaphore.h"

#include "DX12GraphicsCard.h"
#include "slag/exceptions/ResourceCreationError.h"

namespace slag
{
    namespace dx12
    {
        DX12Semaphore::DX12Semaphore(DX12GraphicsCard* graphicsCard, uint64_t initialValue)
        {
            _graphicsCard = graphicsCard;
            if (graphicsCard->device()->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence))!= S_OK)
            {
                throw ResourceCreationError("Unable to create Semaphore");
            }
        }

        DX12Semaphore::DX12Semaphore(DX12Semaphore&& from) noexcept
        {
            move(from);
        }

        DX12Semaphore& DX12Semaphore::operator=(DX12Semaphore&& from) noexcept
        {
            move(from);
            return *this;
        }

        DX12Semaphore::~DX12Semaphore()
        {
            if (_fence)
            {
                _fence->Release();
            }
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

        GraphicsCard* DX12Semaphore::graphicsCard() const
        {
            return _graphicsCard;
        }

        ID3D12Fence* DX12Semaphore::dx12Handle() const
        {
            return _fence;
        }

        void DX12Semaphore::move(DX12Semaphore& from)
        {
            std::swap(_fence, from._fence);
            std::swap(_graphicsCard, from._graphicsCard);
        }
    } // dx12
} // slag
