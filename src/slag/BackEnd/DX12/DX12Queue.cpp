#include <cassert>
#include "DX12Queue.h"
#include "DX12CommandBuffer.h"
#include "DX12Lib.h"

namespace slag
{
    namespace dx
    {

        DX12Queue::DX12Queue(ID3D12CommandQueue* queue, GpuQueue::QueueType type)
        {
            _queue = queue;
        }

        DX12Queue::~DX12Queue()
        {
            if(_queue)
            {
                _queue->Release();
            }
        }

        DX12Queue::DX12Queue(DX12Queue&& from)
        {
            move(std::move(from));
        }

        DX12Queue& DX12Queue::operator=(DX12Queue&& from)
        {
            move(std::move(from));
            return *this;
        }

        void DX12Queue::move(DX12Queue&& from)
        {
            std::swap(_queue,from._queue);
            _type = from._type;
        }

        GpuQueue::QueueType DX12Queue::type()
        {
            return _type;
        }

        void DX12Queue::submit(CommandBuffer* commands)
        {
            auto buffer = dynamic_cast<DX12CommandBuffer*>(commands);
            buffer->_finished->signal(0);
            ID3D12CommandList* lists[1]{buffer->_buffer};
            _queue->ExecuteCommandLists(1,lists);
            _queue->Signal(buffer->_finished->fence(),1);
        }

        void DX12Queue::submit(CommandBuffer** commandBuffers, size_t bufferCount, bool forceDependency)
        {
            if(forceDependency)
            {
                for(size_t i=0; i<bufferCount; i++)
                {
                    submit(commandBuffers[i]);
                }
            }
            else
            {
                std::vector<ID3D12CommandList*> commands(bufferCount, nullptr);
                std::vector<ID3D12Fence*> signals(bufferCount, nullptr);
                for(size_t  i =0; i< bufferCount; i++)
                {
                    auto buffer = dynamic_cast<DX12CommandBuffer*>(commandBuffers[i]);
                    buffer->_finished->signal(0);
                    commands[i] = buffer->_buffer;
                    signals[i] = buffer->_finished->fence();
                }
                _queue->ExecuteCommandLists(bufferCount,commands.data());
                for(size_t i=0; i< bufferCount; i++)
                {
                    _queue->Signal(signals[i],1);
                }
            }

        }

        void DX12Queue::submit(CommandBuffer* commands, SemaphoreValue& signalFinished)
        {
            submit(commands);
            auto semaphore = dynamic_cast<DX12Semaphore*>(signalFinished.semaphore);
            _queue->Signal(semaphore->fence(),signalFinished.value);
        }

        void DX12Queue::submit(CommandBuffer** commandBuffers, size_t bufferCount, SemaphoreValue& signalFinished, bool forceDependency)
        {
            submit(commandBuffers,bufferCount,forceDependency);
            auto semaphore = dynamic_cast<DX12Semaphore*>(signalFinished.semaphore);
            _queue->Signal(semaphore->fence(),signalFinished.value);
        }

        void DX12Queue::submit(CommandBuffer** commandBuffers, size_t bufferCount, SemaphoreValue* waitOnSemaphores, size_t waitCount, SemaphoreValue* signalSemaphores, size_t signalCount)
        {
            for(size_t i=0; i < waitCount; i++)
            {
                auto semaphore = dynamic_cast<DX12Semaphore*>(waitOnSemaphores[i].semaphore);
                _queue->Wait(semaphore->fence(),waitOnSemaphores[i].value);
            }
            std::vector<ID3D12CommandList*> buffers(bufferCount, nullptr);
            for(size_t i=0; i< bufferCount; i++)
            {
                buffers[i] = dynamic_cast<DX12CommandBuffer*>(commandBuffers[i])->_buffer;
            }
            _queue->ExecuteCommandLists(bufferCount,buffers.data());
            for(size_t i=0; i< signalCount; i++)
            {
                auto semaphore = dynamic_cast<DX12Semaphore*>(signalSemaphores[i].semaphore);
                _queue->Signal(semaphore->fence(),signalSemaphores[i].value);
            }
        }
    } // dx
} // slag