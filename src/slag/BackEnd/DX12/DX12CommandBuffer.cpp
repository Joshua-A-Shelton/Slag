#include "DX12CommandBuffer.h"
#include "DX12Lib.h"
namespace slag
{
    namespace dx
    {
        DX12CommandBuffer::DX12CommandBuffer(D3D12_COMMAND_LIST_TYPE type)
        {
            DX12Lib::card()->device()->CreateCommandAllocator(type, IID_PPV_ARGS(&_pool));
            DX12Lib::card()->device()->CreateCommandList(0,type,_pool, nullptr, IID_PPV_ARGS(&_buffer));
            _finished = new DX12Semaphore(2,true);
        }

        DX12CommandBuffer::~DX12CommandBuffer()
        {

            if(_buffer)
            {
                //there's the possibility we started recording commands, but never submitted. Force clear resources just in case
                if(_finished->value() == 2)
                {
                    resources::ResourceManager::removeBufferFromActive(this);
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
            std::swap(_buffer,from._buffer);
            std::swap(_pool,from._pool);
            std::swap(_finished,from._finished);
        }

        void DX12CommandBuffer::begin()
        {
            _waitUntilFinished();
            resources::ResourceManager::setBufferAsActive(this);
            _buffer->Reset(_pool, nullptr);
        }

        void DX12CommandBuffer::end()
        {
            _buffer->Close();
        }

        void DX12CommandBuffer::waitUntilFinished()
        {
            _waitUntilFinished();
        }

        bool DX12CommandBuffer::isFinished()
        {
            return _finished->value() >=1;
        }

        void DX12CommandBuffer::_waitUntilFinished()
        {
            if(_finished->value()==0)
            {
                _finished->waitForValue(1);
            }
            else if(_finished->value()==1)
            {
                resources::ResourceManager::removeBufferFromActive(this);
                freeResourceReferences();
                _finished->signal(2);
            }
        }
    } // dx
} // slag