#include "DX12Buffer.h"

#include "DX12CommandBuffer.h"
#include "DX12GraphicsCard.h"
#include "DX12Semaphore.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace dx12
    {
        DX12Buffer::DX12Buffer(size_t size, Accessibility accessibility, UsageFlags usage)
        {
            _size = size;
            _accessibility = accessibility;
            _usage = usage;


            D3D12MA::Pool* manualPool = nullptr;
            //TODO: I may want to specify D3D12_HEAP_TYPE_CUSTOM, and assign the properties manually. see https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_heap_type
            D3D12_HEAP_TYPE heapType=D3D12_HEAP_TYPE_DEFAULT;

            if(_accessibility == Buffer::Accessibility::CPU_AND_GPU)
            {
                heapType = D3D12_HEAP_TYPE_CUSTOM;
                manualPool = DX12GraphicsCard::selected()->sharedMemoryPool();
            }

            D3D12_RESOURCE_DESC resourceDesc = {};
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            resourceDesc.Alignment = 0;
            resourceDesc.Width = _size;
            resourceDesc.Height = 1;
            resourceDesc.DepthOrArraySize = 1;
            resourceDesc.MipLevels = 1;
            resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
            resourceDesc.SampleDesc.Count = 1;
            resourceDesc.SampleDesc.Quality = 0;
            resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

            D3D12MA::ALLOCATION_DESC allocationDesc = {};

            allocationDesc.HeapType = heapType;
            allocationDesc.CustomPool = manualPool;

            DX12GraphicsCard::selected()->allocator()->CreateResource(&allocationDesc,&resourceDesc,D3D12_RESOURCE_STATE_COMMON, nullptr,&_allocation, IID_PPV_ARGS(&_buffer));

            if (_accessibility == Buffer::Accessibility::CPU_AND_GPU)
            {
                _buffer->Map(0,nullptr,&_cpuHandle);
            }

        }

        DX12Buffer::DX12Buffer(void* data, size_t size, Accessibility accessibility, UsageFlags usage):DX12Buffer(size,accessibility,usage)
        {
            DX12Buffer::update(0,data,size,nullptr,0,nullptr,0);
        }

        DX12Buffer::~DX12Buffer()
        {
            if (_buffer != nullptr)
            {
                if (_accessibility == Accessibility::CPU_AND_GPU)
                {
                    _buffer->Unmap(0, nullptr);
                }
                _buffer->Release();
                _allocation->Release();
            }
        }

        DX12Buffer::DX12Buffer(DX12Buffer&& from)
        {
            move(from);
        }

        DX12Buffer& DX12Buffer::operator=(DX12Buffer&& from)
        {
            move(from);
            return *this;
        }

        Buffer::Accessibility DX12Buffer::accessibility()
        {
            return _accessibility;
        }

        uint64_t DX12Buffer::size()
        {
            return _size;
        }

        Buffer::UsageFlags DX12Buffer::usage()
        {
            return _usage;
        }

        void DX12Buffer::update(uint64_t offset, void* data, uint64_t dataLength, SemaphoreValue* wait,
            uint32_t waitCount, SemaphoreValue* signal, uint32_t signalCount)
        {
            if (_accessibility == Accessibility::CPU_AND_GPU)
            {
                cpuUpdate(offset, data, dataLength, wait, waitCount, signal, signalCount);
            }
            else
            {
                gpuUpdate(offset, data, dataLength, wait, waitCount, signal, signalCount);
            }
        }

        void* DX12Buffer::cpuHandle()
        {
            SLAG_ASSERT(_accessibility == Accessibility::CPU_AND_GPU && "Buffer must be CPU accessible");
            return _cpuHandle;
        }

        ID3D12Resource* DX12Buffer::dx12Handle()
        {
            return _buffer;
        }

        void DX12Buffer::move(DX12Buffer& from)
        {
            std::swap(_buffer,from._buffer);
            std::swap( _size, from._size);
            std::swap( _allocation, from._allocation);
            std::swap( _accessibility, from._accessibility);
            std::swap( _usage, from._usage);
            std::swap( _cpuHandle,from._cpuHandle);
        }

        void DX12Buffer::cpuUpdate(uint64_t offset, void* data, uint64_t dataLength, SemaphoreValue* wait,
            uint32_t waitCount, SemaphoreValue* signal, uint32_t signalCount)
        {
            SLAG_ASSERT(offset + dataLength <= _size && "Update exceeds size of buffer");
            if (waitCount)
            {
                SLAG_ASSERT(wait != nullptr);
                Semaphore::waitFor(wait,waitCount);
            }
            memcpy(static_cast<unsigned char*>(_cpuHandle)+offset,data,dataLength);
            if (signalCount)
            {
                SLAG_ASSERT(signal != nullptr);
                for (size_t i=0; i< signalCount; i++)
                {
                    auto& semaphore = signal[i];
                    semaphore.semaphore->signal(semaphore.value);
                }
            }
        }

        void DX12Buffer::gpuUpdate(uint64_t offset, void* data, uint64_t dataLength, SemaphoreValue* wait,
            uint32_t waitCount, SemaphoreValue* signal, uint32_t signalCount)
        {
            SLAG_ASSERT(offset + dataLength <= _size && "Update exceeds size of buffer");
            DX12CommandBuffer commandBuffer(GPUQueue::QueueType::TRANSFER);
            DX12Semaphore finished(0);
            //this should technically be faster to not send that data initially, as we're not waiting on any semaphores this way
            DX12Buffer buffer(dataLength,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::DATA_BUFFER);
            memcpy(buffer._cpuHandle,data,dataLength);

            commandBuffer.begin();
            commandBuffer.copyBufferToBuffer(&buffer,0,dataLength,this,offset);
            commandBuffer.end();

            CommandBuffer* ptr = &commandBuffer;
            std::vector<SemaphoreValue> signals(signalCount+1);
            for (int i=0; i< signalCount; i++)
            {
                signals[i] = signal[i];
            }
            signals[signalCount] = {.semaphore = &finished,.value = 1};
            QueueSubmissionBatch submissionData
            {
                .waitSemaphores = wait,
                .waitSemaphoreCount = waitCount,
                .commandBuffers = &ptr,
                .commandBufferCount = 1,
                .signalSemaphores = signals.data(),
                .signalSemaphoreCount = signalCount+1,
            };
            DX12GraphicsCard::selected()->transferQueue()->submit(&submissionData,1);
            finished.waitForValue(1);
        }
    } // dx12
} // slag
