#include "DX12Buffer.h"

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
            BufferUsage usage,
            BufferShaderAccess shaderAccess,
            BufferCPUAccess cpuAccess)
        {
            SLAG_ASSERT(((shaderAccess == BufferShaderAccess::READ_ONLY && size%256 == 0) || shaderAccess != BufferShaderAccess::READ_ONLY) && "Buffers with BufferShaderAccess::READ_ONLY must be a multiple of 256 bytes in size");
            SLAG_ASSERT(((shaderAccess == BufferShaderAccess::READ_ONLY && size<= card->maxShaderAccessReadOnlyBufferSize()) || shaderAccess != BufferShaderAccess::READ_ONLY) && "Buffers with BufferShaderAccess::READ_ONLY cannot exceed size found in GraphicsCard::maxShaderAccessReadOnlyBufferSize");

            _graphicsCard = card;
            _size = size;
            _dataBits = static_cast<uint16_t>(usage);
            _dataBits |= static_cast<uint16_t>(shaderAccess)<<DXBUFFER_SHADER_SHIFT;
            _dataBits |= static_cast<uint16_t>(cpuAccess)<<DXBUFFER_CPU_SHIFT;

            D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;
            switch (cpuAccess)
            {
            case BufferCPUAccess::NONE:
                heapType = D3D12_HEAP_TYPE_DEFAULT;
                break;
            case BufferCPUAccess::WRITE_ONLY:
                if (card->cacheCoherentSharedMemory())
                {
                    heapType = D3D12_HEAP_TYPE_GPU_UPLOAD;
                }
                else
                {
                    heapType = D3D12_HEAP_TYPE_UPLOAD;
                }
                break;
            case BufferCPUAccess::READ_WRITE:
                heapType = D3D12_HEAP_TYPE_READBACK;
                break;
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

            D3D12MA::ALLOCATION_DESC allocationDesc{};

            allocationDesc.HeapType = heapType;

            if (_graphicsCard->allocator()->CreateResource(&allocationDesc,&resourceDesc,D3D12_RESOURCE_STATE_COMMON, nullptr,&_allocation, IID_PPV_ARGS(&_buffer)) != S_OK)
            {
                throw ResourceCreationError("Unable to create buffer");
            }

            if (cpuAccess != BufferCPUAccess::NONE)
            {
                _buffer->Map(0,nullptr,&_cpuHandle);
            }
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

        void* DX12Buffer::data() const
        {
            SLAG_ASSERT(cpuAccess()!=BufferCPUAccess::NONE && "Attempted to access cpu handle of inaccessible buffer");
            return _cpuHandle;
        }

        uint64_t DX12Buffer::deviceAddress() const
        {
            return  _buffer->GetGPUVirtualAddress();
        }

        BufferUsage DX12Buffer::usage() const
        {
            return static_cast<BufferUsage>(_dataBits & DXBUFFER_USAGE_BITS);
        }

        BufferShaderAccess DX12Buffer::shaderAccess() const
        {
            return static_cast<BufferShaderAccess>((_dataBits & DXBUFFER_SHADER_BITS) >> DXBUFFER_SHADER_SHIFT);
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

        void DX12Buffer::move(DX12Buffer& from)
        {
            _size = from._size;
            std::swap(_buffer,from._buffer);
            std::swap(_allocation, from._allocation);
            std::swap(_graphicsCard,from._graphicsCard);
            std::swap(_cpuHandle,from._cpuHandle);
            _dataBits = from._dataBits;
        }
    } // dx12
} // slag
