#ifndef SLAG_DX12QUEUE_H
#define SLAG_DX12QUEUE_H
#include <slag/Slag.h>
#include <wrl/client.h>

#include "D3D12MemAlloc.h"

namespace slag
{
    namespace dx12
    {
        class DX12Queue: public GPUQueue
        {
        public:
            DX12Queue(Microsoft::WRL::ComPtr<ID3D12Device2>& device,GPUQueue::QueueType queueType);
            virtual ~DX12Queue()override;
            ///The actual type this queue is, as lower queues commands can be processed by higher queues
            virtual QueueType type()override;

            /**
             * Submit command buffers for execution
             * @param submissionData Information to submit to the queue
             * @param submissionDataCount Number of Submission Data
             */
            virtual void submit(QueueSubmissionBatch* submissionData, uint32_t submissionDataCount)override;
            /**
             * Submit command buffers for execution and frame for presentation
             * @param submissionData Information to submit to the queue
             * @param submissionDataCount Number of Submission Data
             * @param frame The frame to present after the buffers execute
             */
            virtual void submit(QueueSubmissionBatch* submissionData, uint32_t submissionDataCount,Frame* frame)override;
        private:
            ID3D12CommandQueue* _queue = nullptr;
            GPUQueue::QueueType _queueType = GPUQueue::QueueType::GRAPHICS;
        };
    } // dx12
} // slag

#endif //SLAG_DX12QUEUE_H
