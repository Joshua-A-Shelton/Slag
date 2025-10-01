#ifndef SLAG_DX12GRAPHICSCARD_H
#define SLAG_DX12GRAPHICSCARD_H
#include <d3d12.h>
#include <dxgi1_6.h>
#include <queue>
#include <slag/Slag.h>
#include <wrl/client.h>

#include "D3D12MemAlloc.h"
#include "DX12Queue.h"

namespace slag
{
    namespace dx12
    {
        class DX12GraphicsCard: public GraphicsCard
        {
        public:
            DX12GraphicsCard(Microsoft::WRL::ComPtr<IDXGIAdapter4>& adapter, Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory);
            ///Total video memory in bytes
            virtual uint64_t videoMemory()override;
            ///Name of graphics card
            virtual std::string name()override;

            ///Gets the graphics queue
            virtual GPUQueue* graphicsQueue()override;
            ///Gets the compute queue, or a default queue that processes it's commands
            virtual GPUQueue* computeQueue()override;
            ///Gets the transfer queue, or a queue that processes it's commands
            virtual GPUQueue* transferQueue()override;

            ///Alignment requirement when binding uniform buffer memory, (eg DescriptorBundle::setUniformBuffer(uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t *offset*, size_t length) and similar calls with an offset must be a multiple of this number)
            virtual uint64_t uniformBufferOffsetAlignment()override;
            ///Alignment requirement when binding storage buffer memory, (eg DescriptorBundle::setStorageBuffer(uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t *offset*, size_t length) and similar calls with an offset must be a multiple of this number)
            virtual uint64_t storageBufferOffsetAlignment()override;

            ///Defragment video memory, blocks until finished
            virtual void defragmentMemory(SemaphoreValue* waitFor, size_t waitForCount, SemaphoreValue* signal, size_t signalCount)override;

            bool isValidGraphicsCard()const;

            static DX12GraphicsCard* selected();
            Microsoft::WRL::ComPtr<ID3D12Device2> device()const;
            ID3D12DescriptorHeap* samplerHeap()const;
            D3D12MA::Allocator* allocator()const;
            D3D12MA::Pool* sharedMemoryPool()const;

            D3D12_CPU_DESCRIPTOR_HANDLE getSamplerHandle();
            void freeSamplerHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle);

        private:
            Microsoft::WRL::ComPtr<ID3D12Device2> _device = nullptr;
            Microsoft::WRL::ComPtr<IDXGIFactory4> _dxgiFactory = nullptr;
            DX12Queue* _graphics = nullptr;
            DX12Queue* _compute = nullptr;
            DX12Queue* _transfer = nullptr;
            D3D12MA::Allocator* _allocator = nullptr;
            D3D12MA::Pool* _sharedMemoryPool = nullptr;
            ID3D12DescriptorHeap* _samplerHeap = nullptr;
            int _samplerIndex = 0;
            std::queue<D3D12_CPU_DESCRIPTOR_HANDLE> _freedSamplerHandles;
            bool _validGraphicsCard = false;
        };
    } // dx12
} // slag

#endif //SLAG_DX12GRAPHICSCARD_H
