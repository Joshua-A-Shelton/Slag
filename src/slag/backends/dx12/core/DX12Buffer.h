#ifndef SLAG_DX12BUFFER_H
#define SLAG_DX12BUFFER_H

#include <slag/Slag.h>
#include <d3d12.h>
#include <D3D12MemAlloc.h>

namespace slag
{
    namespace dx12
    {
        class DX12Buffer: public Buffer
        {
        public:
            DX12Buffer(size_t size, Accessibility accessibility,UsageFlags usage);
            DX12Buffer(void* data, size_t size, Accessibility accessibility,UsageFlags usage);
            virtual ~DX12Buffer()override;
            DX12Buffer(const DX12Buffer&) = delete;
            DX12Buffer& operator=(const DX12Buffer&) = delete;
            DX12Buffer(DX12Buffer&& from);
            DX12Buffer& operator=(DX12Buffer&& from);
            ///Who can natively access this buffer
            virtual Accessibility accessibility()override;
            ///Size in bytes of the buffer
            virtual uint64_t size()override;
            ///What kind of data this buffer can contain
            virtual UsageFlags usage()override;

            /**
             * Send data to buffer from CPU (goes through intermediate buffer automatically if required)
             * @param offset position in the buffer to start updating
             * @param data the new data to put into the buffer
             * @param dataLength length of new data being inserted into the buffer
             * @param wait semaphores to wait to be signaled before performing the update
             * @param waitCount number of semaphores in wait array
             * @param signal semaphores to signal when update is finished
             * @param signalCount number of semaphores in signal array
             */
            virtual void update(uint64_t offset, void* data, uint64_t dataLength,SemaphoreValue* wait, uint32_t waitCount,SemaphoreValue* signal, uint32_t signalCount)override;
            ///The location of the buffer in host memory (if cpu accessible)
            virtual void* cpuHandle()override;

            ID3D12Resource* dx12Handle();
        protected:
            void move(DX12Buffer& from);
        private:
            void cpuUpdate(uint64_t offset, void* data, uint64_t dataLength,SemaphoreValue* wait, uint32_t waitCount,SemaphoreValue* signal, uint32_t signalCount);
            void gpuUpdate(uint64_t offset, void* data, uint64_t dataLength,SemaphoreValue* wait, uint32_t waitCount,SemaphoreValue* signal, uint32_t signalCount);
            ID3D12Resource* _buffer = nullptr;
            uint64_t _size = 0;
            D3D12MA::Allocation* _allocation = nullptr;
            Accessibility _accessibility = Accessibility::CPU_AND_GPU;
            UsageFlags _usage = UsageFlags::DATA_BUFFER;
            void* _cpuHandle = nullptr;
        };
    } // dx12
} // slag

#endif //SLAG_DX12BUFFER_H
