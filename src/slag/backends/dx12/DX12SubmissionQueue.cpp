#include "DX12SubmissionQueue.h"

#include <vector>

#include "DX12CommandBuffer.h"
#include "DX12GraphicsCard.h"
#include "DX12Semaphore.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace dx12
    {
        DX12SubmissionQueue::DX12SubmissionQueue(DX12GraphicsCard* graphicsCard, QueueType type)
        {
            _graphicsCard = graphicsCard;
            _type = type;

            D3D12_COMMAND_QUEUE_DESC desc = {};
            switch (type)
            {
            case QueueType::GRAPHICS:
                desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
                desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
                break;
            case QueueType::COMPUTE:
                desc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
                desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
                break;
            case QueueType::TRANSFER:
                desc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
                desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
                break;
            }
            desc.Flags =    D3D12_COMMAND_QUEUE_FLAG_NONE;
            desc.NodeMask = 0;


            _graphicsCard->device()->CreateCommandQueue(&desc, IID_PPV_ARGS(&_queue));

        }

        DX12SubmissionQueue::DX12SubmissionQueue(DX12SubmissionQueue&& from) noexcept
        {
            move(from);
        }

        DX12SubmissionQueue& DX12SubmissionQueue::operator=(DX12SubmissionQueue&& from) noexcept
        {
            move(from);
            return *this;
        }

        QueueType DX12SubmissionQueue::type() const
        {
            return _type;
        }

        GraphicsCard* DX12SubmissionQueue::graphicsCard()
        {
            return _graphicsCard;
        }

        void DX12SubmissionQueue::submit(SubmissionBatch* batches, uint32_t batchCount)
        {
            SLAG_ASSERT(batchCount > 0 && "At least one batch must be submitted");
            SLAG_ASSERT(batches != nullptr && "Parameter 'batches' must not be nullptr");
            for (auto submissionIndex = 0; submissionIndex < batchCount; ++submissionIndex)
            {
                auto& submission = batches[submissionIndex];
                for (auto waitIndex = 0; waitIndex < submission.waitSemaphoreCount; ++waitIndex)
                {
                    auto& waitSemaphore = submission.waitSemaphores[waitIndex];
                    _queue->Wait(static_cast<DX12Semaphore*>(waitSemaphore.semaphore)->dx12Handle(),waitSemaphore.value);
                }
                std::vector<ID3D12CommandList*> buffers(submission.commandBufferCount, nullptr);
                for (auto bufferIndex = 0; bufferIndex < submission.commandBufferCount; ++bufferIndex)
                {

                    buffers[bufferIndex] = static_cast<DX12CommandBuffer*>(submission.commandBuffers[bufferIndex])->dx12Handle();
                    SLAG_ASSERT(QueueTypeSupportsCommands(_type,submission.commandBuffers[bufferIndex]->type()) && "Queue cannot process command buffer outside it's capabilities");
                }
                _queue->ExecuteCommandLists(buffers.size(), buffers.data());
                for (auto signalIndex = 0; signalIndex < submission.signalSemaphoreCount; ++signalIndex)
                {
                    auto& signalSemaphore = submission.signalSemaphores[signalIndex];
                    _queue->Signal(static_cast<DX12Semaphore*>(signalSemaphore.semaphore)->dx12Handle(),signalSemaphore.value);
                }
            }
        }

        ID3D12CommandQueue* DX12SubmissionQueue::dx12Handle() const
        {
            return _queue;
        }

        void DX12SubmissionQueue::move(DX12SubmissionQueue& from)
        {
            std::swap(_graphicsCard,from._graphicsCard);
            std::swap(_queue,from._queue);
            _type = from._type;
        }
    } // dx12
} // slag
