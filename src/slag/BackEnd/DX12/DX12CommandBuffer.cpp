#include "DX12CommandBuffer.h"
#include "DX12Lib.h"
#include "../../Resources/ResourceManager.h"
#include "DX12DescriptorPool.h"

namespace slag
{
    namespace dx
    {
        DX12CommandBuffer::DX12CommandBuffer(GpuQueue::QueueType commandType)
        {
            _commandType = commandType;
            D3D12_COMMAND_LIST_TYPE comType = D3D12_COMMAND_LIST_TYPE_DIRECT;
            switch (_commandType)
            {
                case GpuQueue::GRAPHICS:
                    comType = D3D12_COMMAND_LIST_TYPE_DIRECT;
                    break;
                case GpuQueue::TRANSFER:
                    comType = D3D12_COMMAND_LIST_TYPE_COPY;
                    break;
                case GpuQueue::COMPUTE:
                    comType = D3D12_COMMAND_LIST_TYPE_COMPUTE;
                    break;

            }
            DX12Lib::card()->device()->CreateCommandAllocator(comType, IID_PPV_ARGS(&_pool));
            DX12Lib::card()->device()->CreateCommandList(0,comType,_pool, nullptr, IID_PPV_ARGS(&_buffer));
            _buffer->Close();
        }

        DX12CommandBuffer::~DX12CommandBuffer()
        {

            if(_buffer)
            {

                //there's the possibility we started recording commands, but never submitted. Force clear resources just in case
                if(_finished == nullptr)
                {
                    resources::ResourceManager::removeConsumerFromActive(this);
                    freeResourceReferences();
                }
                else
                {
                    _waitUntilFinished();
                }
                _buffer->Release();
                _pool->Release();
                delete _finished;
            }
        }

        DX12CommandBuffer::DX12CommandBuffer(DX12CommandBuffer&& from)
        {
            move(std::move(from));
        }

        DX12CommandBuffer& DX12CommandBuffer::operator=(DX12CommandBuffer&& from)
        {
            move(std::move(from));
            return *this;
        }

        void DX12CommandBuffer::move(DX12CommandBuffer&& from)
        {
            IDX12CommandBuffer::move(from);
            ResourceConsumer::move(from);
            std::swap(_finished,from._finished);
        }

        void DX12CommandBuffer::begin()
        {
            _waitUntilFinished();
            resources::ResourceManager::setConsumerAsActive(this);
            _buffer->Reset(_pool, nullptr);
        }

        void DX12CommandBuffer::waitUntilFinished()
        {
            _waitUntilFinished();
        }

        bool DX12CommandBuffer::isFinished()
        {
            if(_finished)
            {
                return _finished->value() >=1;
            }
            return true;
        }

        void DX12CommandBuffer::_waitUntilFinished()
        {
            if(_finished)
            {
                _finished->waitForValue(1);
                resources::ResourceManager::removeConsumerFromActive(this);
                freeResourceReferences();
                delete _finished;
                _finished = nullptr;
            }
        }

        void DX12CommandBuffer::bindDescriptorPool(DescriptorPool* pool)
        {
            auto descriptorPool = static_cast<DX12DescriptorPool*>(pool);
            //_buffer->SetDescriptorHeaps(2,)
            throw std::runtime_error("DX12CommandBuffer::bindDescriptorPool not implemented");
        }

    } // dx
} // slag