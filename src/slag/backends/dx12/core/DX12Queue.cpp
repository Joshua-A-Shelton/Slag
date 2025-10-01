#include "DX12Queue.h"

#include <iostream>

#include "DX12CommandBuffer.h"
#include "DX12Semaphore.h"

namespace slag
{
    namespace dx12
    {
        DX12Queue::DX12Queue(Microsoft::WRL::ComPtr<ID3D12Device2>& device, GPUQueue::QueueType queueType)
        {
            D3D12_COMMAND_QUEUE_DESC desc = {};
            switch (queueType)
            {
                case GPUQueue::QueueType::GRAPHICS:
                    desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
                    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
                    break;
                case GPUQueue::QueueType::COMPUTE:
                    desc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
                    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
                    break;
                case GPUQueue::QueueType::TRANSFER:
                    desc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
                    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
                    break;
            }
            desc.Flags =    D3D12_COMMAND_QUEUE_FLAG_NONE;
            desc.NodeMask = 0;


            device->CreateCommandQueue(&desc, IID_PPV_ARGS(&_queue));
            _queueType = queueType;
        }

        DX12Queue::~DX12Queue()
        {
           _queue->Release();
        }

        GPUQueue::QueueType DX12Queue::type()
        {
            return _queueType;
        }

        void DX12Queue::submit(QueueSubmissionBatch* submissionData, uint32_t submissionDataCount)
        {
            for (auto submissionIndex = 0; submissionIndex < submissionDataCount; ++submissionIndex)
            {
                auto& submission = submissionData[submissionIndex];
                for (auto waitIndex = 0; waitIndex < submission.waitSemaphoreCount; ++waitIndex)
                {
                    auto& waitSemaphore = submission.waitSemaphores[waitIndex];
                    _queue->Wait(static_cast<DX12Semaphore*>(waitSemaphore.semaphore)->dx12Handle(),waitSemaphore.value);
                }
                std::vector<ID3D12CommandList*> buffers(submission.commandBufferCount, nullptr);
                for (auto bufferIndex = 0; bufferIndex < submission.commandBufferCount; ++bufferIndex)
                {
                    buffers[bufferIndex] = static_cast<DX12CommandBuffer*>(submission.commandBuffers[bufferIndex])->dx12Handle();
                }
                _queue->ExecuteCommandLists(buffers.size(), buffers.data());
                for (auto signalIndex = 0; signalIndex < submission.signalSemaphoreCount; ++signalIndex)
                {
                    auto& signalSemaphore = submission.signalSemaphores[signalIndex];
                    _queue->Signal(static_cast<DX12Semaphore*>(signalSemaphore.semaphore)->dx12Handle(),signalSemaphore.value);
                }
            }
        }

        void DX12Queue::submit(QueueSubmissionBatch* submissionData, uint32_t submissionDataCount, Frame* frame)
        {
            throw std::runtime_error("DX12Queue::submit(): not implemented");
        }
    } // dx12
} // slag
