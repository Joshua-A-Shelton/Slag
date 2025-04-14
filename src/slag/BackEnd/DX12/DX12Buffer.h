#ifndef SLAG_DX12BUFFER_H
#define SLAG_DX12BUFFER_H

#include <directx/d3d12.h>
#include "../../Buffer.h"
#include "../../Resources/Resource.h"
#include "D3D12MemAlloc.h"

namespace slag
{
    namespace dx
    {

        class DX12Buffer: public Buffer, resources::Resource
        {
        public:
            DX12Buffer(void* data, size_t dataLength, Accessibility accessibility, D3D12_RESOURCE_STATES usageStateFlags, bool destroyImmediately);
            DX12Buffer(size_t bufferSize, Accessibility accessibility, D3D12_RESOURCE_STATES usageStateFlags, bool destroyImmediately);
            ~DX12Buffer();
            DX12Buffer(const DX12Buffer&)=delete;
            DX12Buffer& operator=(const DX12Buffer&)=delete;
            DX12Buffer(DX12Buffer&& from);
            DX12Buffer& operator=(DX12Buffer&& from);

            void update(size_t offset, void* data, size_t dataLength)override;
            std::vector<std::byte> downloadData()override;
            size_t size()override;
            Accessibility accessibility()override;
            unsigned char* cpuHandle()override;
            ID3D12Resource* underlyingBuffer();

        private:
            void move(DX12Buffer&& from);
            void updateCPU(size_t offset, void* data, size_t dataLength);
            void updateGPU(size_t offset, void* data, size_t dataLength);
            std::vector<std::byte> downloadDataCPU();
            std::vector<std::byte> downloadDataGPU();
            ID3D12Resource* _buffer = nullptr;
            size_t _size = 0;
            D3D12MA::Allocation* _allocation = nullptr;
            Accessibility _accessibility=CPU_AND_GPU;
            void* _memoryLocation = nullptr;
        };

    } // dx
} // slag

#endif //SLAG_DX12BUFFER_H
