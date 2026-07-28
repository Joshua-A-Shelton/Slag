#ifndef SLAG_DX12SUBMISSIONQUEUE_H
#define SLAG_DX12SUBMISSIONQUEUE_H
#include <slag/Slag.h>
#include <d3d12.h>
namespace slag
{
    namespace dx12
    {
        class DX12GraphicsCard;

        class DX12SubmissionQueue: public SubmissionQueue
        {
        public:
            DX12SubmissionQueue(DX12GraphicsCard* graphicsCard, QueueType type);
            DX12SubmissionQueue(const DX12SubmissionQueue&)=delete;
            DX12SubmissionQueue& operator=(const DX12SubmissionQueue&)=delete;
            DX12SubmissionQueue(DX12SubmissionQueue&& from) noexcept;
            DX12SubmissionQueue& operator=(DX12SubmissionQueue&& from) noexcept;
            ~DX12SubmissionQueue()override = default;
            [[nodiscard]] QueueType type()const override;
            [[nodiscard]] GraphicsCard* graphicsCard()override;
            void submit(SubmissionBatch* batches, uint32_t batchCount)override;
            ID3D12CommandQueue* dx12Handle()const;
        private:
            void move(DX12SubmissionQueue& from);
            DX12GraphicsCard* _graphicsCard = nullptr;
            ID3D12CommandQueue* _queue = nullptr;
            QueueType _type = QueueType::TRANSFER;
        };
    } // dx12
} // slag

#endif //SLAG_DX12SUBMISSIONQUEUE_H
