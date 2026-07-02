#include "DX12Buffer.h"

#include "DX12CommandBuffer.h"
#include "DX12GraphicsCard.h"
#include "slag/exceptions/ResourceCreationError.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace dx12
    {
        DX12Buffer::DX12Buffer(
            DX12GraphicsCard* card,
            uint64_t size,
            BufferCPUAccess cpuAccess,
            BufferMemoryType shaderAccess)
        {
            SLAG_ASSERT(((shaderAccess == BufferMemoryType::UNIFORM && size%256 == 0) || shaderAccess != BufferMemoryType::UNIFORM) && "Buffers with BufferMemoryType::UNIFORM must be a multiple of 256 bytes in size");
            SLAG_ASSERT(((shaderAccess == BufferMemoryType::UNIFORM && size<= card->memoryProperties().maxUniformBufferSize) || shaderAccess != BufferMemoryType::UNIFORM) && "Buffers with BufferMemoryType::UNIFORM cannot exceed size found in GraphicsCard::memoryProperties::maxUniformBufferSize");

            _graphicsCard = card;
            _size = size;
            _dataBits = static_cast<uint16_t>(shaderAccess);
            _dataBits |= static_cast<uint16_t>(cpuAccess)<<DXBUFFER_CPU_SHIFT;

            D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;
            D3D12MA::Pool* manualPool = nullptr;
            switch (cpuAccess)
            {
            case BufferCPUAccess::NONE:
                heapType = D3D12_HEAP_TYPE_DEFAULT;
                break;
            case BufferCPUAccess::WRITE_ONLY:
                if (card->memoryProperties().cacheCoherentSharedMemory)
                {
                    heapType = D3D12_HEAP_TYPE_GPU_UPLOAD;
                }
                else
                {
                    //heapType = D3D12_HEAP_TYPE_UPLOAD;
                    heapType = D3D12_HEAP_TYPE_CUSTOM;
                    manualPool = card->cpuReadablePool();
                }
                break;
            case BufferCPUAccess::READ_WRITE:
                heapType = D3D12_HEAP_TYPE_CUSTOM;
                manualPool = card->cpuReadablePool();
                break;
            }

            D3D12_RESOURCE_DESC1 resourceDesc = {};
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
            if (shaderAccess == BufferMemoryType::GENERAL)
            {
                resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            }

            D3D12MA::ALLOCATION_DESC allocationDesc{};

            allocationDesc.HeapType = heapType;
            allocationDesc.CustomPool = manualPool;


            auto result = _graphicsCard->allocator()->CreateResource3(&allocationDesc,&resourceDesc,D3D12_BARRIER_LAYOUT_UNDEFINED,nullptr,0,nullptr,&_allocation, IID_PPV_ARGS(&_buffer));
            if (result != S_OK)
            {
                throw ResourceCreationError("Unable to create buffer");
            }

            if (cpuAccess != BufferCPUAccess::NONE)
            {
                _buffer->Map(0,nullptr,&_cpuHandle);
            }

            _allocation->SetPrivateData(&_selfReference);
        }

        DX12Buffer::~DX12Buffer()
        {
            if (_buffer != nullptr)
            {
                if (DX12Buffer::cpuAccess()!=BufferCPUAccess::NONE)
                {
                    _buffer->Unmap(0, nullptr);
                }
                _buffer->Release();
                _allocation->Release();
            }
        }

        DX12Buffer::DX12Buffer(DX12Buffer&& from) noexcept
        {
            move(from);
        }

        DX12Buffer& DX12Buffer::operator=(DX12Buffer&& from) noexcept
        {
            move(from);
            return *this;
        }

        void* DX12Buffer::pointer() const
        {
            SLAG_ASSERT(cpuAccess()!=BufferCPUAccess::NONE && "Attempted to access cpu handle of inaccessible buffer");
            return _cpuHandle;
        }

        uint64_t DX12Buffer::deviceAddress() const
        {
            return  _buffer->GetGPUVirtualAddress();
        }

        BufferMemoryType DX12Buffer::memoryType() const
        {
            return static_cast<BufferMemoryType>((_dataBits & DXBUFFER_MEMORY_BITS));
        }

        BufferCPUAccess DX12Buffer::cpuAccess() const
        {
            return static_cast<BufferCPUAccess>((_dataBits & DXBUFFER_CPU_BITS) >> DXBUFFER_CPU_SHIFT);
        }

        uint64_t DX12Buffer::size() const
        {
            return _size;
        }

        GraphicsCard* DX12Buffer::graphicsCard() const
        {
            return _graphicsCard;
        }

        void* DX12Buffer::userData()
        {
            return _userData;
        }

        void DX12Buffer::setUserData(void* userData)
        {
            _userData = userData;
        }

        ID3D12Resource* DX12Buffer::dx12Handle() const
        {
            return _buffer;
        }

        ID3D12Resource* DX12Buffer::moveMemory(D3D12MA::Allocation* tempAllocation, CommandBuffer* copyDataBuffer)
        {
            if (DX12Buffer::cpuAccess()!=BufferCPUAccess::NONE)
            {
                _buffer->Unmap(0, nullptr);
            }

            D3D12_RESOURCE_DESC resDesc = _buffer->GetDesc();

            ID3D12Resource* newRes;
            _graphicsCard->device()->CreatePlacedResource(
                tempAllocation->GetHeap(),
                tempAllocation->GetOffset(), &resDesc,
                D3D12_RESOURCE_STATE_COMMON, NULL, IID_PPV_ARGS(&newRes));

            tempAllocation->SetResource(newRes);


            DX12CommandBuffer* cb = static_cast<DX12CommandBuffer*>(copyDataBuffer);
            cb->dx12Handle()->CopyResource(tempAllocation->GetResource(),_buffer);
            ID3D12Resource* returnVal = _buffer;
            _buffer = tempAllocation->GetResource();
            return returnVal;
        }

        void DX12Buffer::updatePointer()
        {
            if (this->cpuAccess() != BufferCPUAccess::NONE)
            {
                _buffer->Map(0,nullptr,&_cpuHandle);
            }
        }

        void DX12Buffer::move(DX12Buffer& from)
        {
            _size = from._size;
            std::swap(_buffer,from._buffer);
            std::swap(_allocation, from._allocation);
            std::swap(_graphicsCard,from._graphicsCard);
            std::swap(_cpuHandle,from._cpuHandle);
            std::swap(_userData,from._userData);
            _dataBits = from._dataBits;

            if (_allocation)
            {
                _allocation->SetPrivateData(&_selfReference);
            }
        }
    } // dx12
} // slag
