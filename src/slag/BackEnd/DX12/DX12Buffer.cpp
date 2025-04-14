#include <cassert>
#include "DX12Buffer.h"
#include "DX12Lib.h"
#include "DX12CommandBuffer.h"

namespace slag
{
    namespace dx
    {
        DX12Buffer::DX12Buffer(void* data, size_t dataLength, Accessibility accessibility, D3D12_RESOURCE_STATES usageStateFlags, bool destroyImmediately): DX12Buffer(dataLength,accessibility,usageStateFlags,destroyImmediately)
        {
            DX12Buffer::update(0,data,dataLength);
        }

        DX12Buffer::DX12Buffer(size_t bufferSize, Accessibility accessibility, D3D12_RESOURCE_STATES usageStateFlags, bool destroyImmediately): resources::Resource(destroyImmediately)
        {
            _size = bufferSize;

            _accessibility = accessibility;
            D3D12MA::Pool* manualPool = nullptr;
            //TODO: I may want to specify D3D12_HEAP_TYPE_CUSTOM, and assign the properties manually. see https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_heap_type
            D3D12_HEAP_TYPE heapType=D3D12_HEAP_TYPE_DEFAULT;
            if(_accessibility == Buffer::Accessibility::CPU)
            {
                heapType = D3D12_HEAP_TYPE_UPLOAD;
            }
            else if(_accessibility & Buffer::Accessibility::CPU)
            {
                heapType = D3D12_HEAP_TYPE_CUSTOM;
                manualPool = DX12Lib::card()->sharedMemoryPool();
            }

            D3D12_RESOURCE_DESC resourceDesc = {};
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            resourceDesc.Alignment = 0;
            resourceDesc.Width = bufferSize;
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

            DX12Lib::card()->allocator()->CreateResource(&allocationDesc,&resourceDesc,usageStateFlags, nullptr,&_allocation, IID_PPV_ARGS(&_buffer));


            auto buffer = _buffer;
            auto alloc = _allocation;
            if(_accessibility & Buffer::Accessibility::CPU)
            {
                _buffer->Map(0, nullptr, &_memoryLocation);
                _disposeFunction = [=]()
                {
                    buffer->Unmap(0, nullptr);
                    buffer->Release();
                    alloc->Release();
                };
            }
            else
            {
                _disposeFunction = [=]()
                {
                    buffer->Release();
                    alloc->Release();
                };
            }

        }

        DX12Buffer::~DX12Buffer()
        {
            if(_buffer)
            {
                smartDestroy();
            }
        }

        DX12Buffer::DX12Buffer(DX12Buffer&& from): resources::Resource(from._destroyImmediately)
        {
            move(std::move(from));
        }

        DX12Buffer& DX12Buffer::operator=(DX12Buffer&& from)
        {
            move(std::move(from));
            return *this;
        }

        void DX12Buffer::move(DX12Buffer&& from)
        {
            resources::Resource::move(from);
            std::swap(_buffer,from._buffer);
            std::swap(_allocation, from._allocation);
            _accessibility = from._accessibility;
        }

        void DX12Buffer::update(size_t offset, void* data, size_t dataLength)
        {
            assert(offset+dataLength <= _size && "update extends beyond bound of buffer");
            if(_accessibility & Accessibility::CPU)
            {
                updateCPU(offset,data,dataLength);
            }
            else
            {
                updateGPU(offset,data,dataLength);
            }
        }

        std::vector<std::byte> DX12Buffer::downloadData()
        {
            if(_accessibility & Accessibility::CPU)
            {
                return downloadDataCPU();
            }
            else
            {
                return downloadDataGPU();
            }
        }

        size_t DX12Buffer::size()
        {
            return _size;
        }

        Buffer::Accessibility DX12Buffer::accessibility()
        {
            return _accessibility;
        }

        unsigned char* DX12Buffer::cpuHandle()
        {
            return static_cast<unsigned char*>(_memoryLocation);
        }

        ID3D12Resource* DX12Buffer::underlyingBuffer()
        {
            return _buffer;
        }

        void DX12Buffer::updateCPU(size_t offset, void* data, size_t dataLength)
        {
            memcpy(((char*)_memoryLocation) + offset, data, dataLength);
        }

        void DX12Buffer::updateGPU(size_t offset, void* data, size_t dataLength)
        {
            DX12Buffer tempBuffer(data,dataLength,Accessibility::CPU,D3D12_RESOURCE_STATE_GENERIC_READ, true);
            DX12CommandBuffer commands(GpuQueue::QueueType::TRANSFER);
            commands.begin();
            commands.copyBuffer(&tempBuffer,0,dataLength,this,offset);
            commands.end();

            DX12Lib::card()->transferQueue()->submit(&commands);
            commands.waitUntilFinished();
        }

        std::vector<std::byte> DX12Buffer::downloadDataCPU()
        {
            std::vector<std::byte> bytes(_size);
            memcpy(bytes.data(),_memoryLocation,_size);
            return bytes;
        }

        std::vector<std::byte> DX12Buffer::downloadDataGPU()
        {
            DX12Buffer temp(_size,Accessibility::CPU_AND_GPU,D3D12_RESOURCE_STATE_COMMON, true);
            DX12CommandBuffer commandBuffer(GpuQueue::QueueType::TRANSFER);
            commandBuffer.begin();
            commandBuffer.copyBuffer(this,0,_size,&temp,0);
            commandBuffer.end();
            DX12Lib::card()->transferQueue()->submit(&commandBuffer);
            commandBuffer.waitUntilFinished();

            return temp.downloadDataCPU();
        }
    } // dx
} // slag