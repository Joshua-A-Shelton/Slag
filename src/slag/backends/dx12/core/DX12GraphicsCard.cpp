#include "DX12GraphicsCard.h"

#include <d3d12.h>

#include "slag/backends/dx12/DX12Backend.h"
#include <D3D12MemAlloc.h>

#include <directx/d3dx12_root_signature.h>

namespace slag
{
    namespace dx12
    {
        void DX12ErrorCallback(D3D12_MESSAGE_CATEGORY category,D3D12_MESSAGE_SEVERITY severity,D3D12_MESSAGE_ID id, LPCSTR pdescription, void* pcontext)
        {
            auto backend = static_cast<DX12Backend*>(Backend::current());
            if(backend->_dx12DebugHandler!=nullptr)
            {
                SlagDebugLevel level = SlagDebugLevel::SLAG_INFO;
                if(severity == D3D12_MESSAGE_SEVERITY_ERROR || severity == D3D12_MESSAGE_SEVERITY_CORRUPTION)
                {
                    level = SlagDebugLevel::SLAG_ERROR;
                }
                else if(severity == D3D12_MESSAGE_SEVERITY_WARNING)
                {
                    level = SlagDebugLevel::SLAG_WARNING;
                }
                std::string message = pdescription;
                backend->_dx12DebugHandler(message,level,id);
            }
        }


        DX12GraphicsCard::DX12GraphicsCard(Microsoft::WRL::ComPtr<IDXGIAdapter4>& adapter,Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory)
        {
            _dxgiFactory = dxgiFactory;
            D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&_device));
            D3D12_FEATURE_DATA_D3D12_OPTIONS12 features{};
            auto res = _device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12,&features,sizeof(features));
            if (!features.EnhancedBarriersSupported)
            {
                _validGraphicsCard = false;
                return;
            }
            D3D12_FEATURE_DATA_D3D12_OPTIONS13 features13{};
            res = _device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS13,&features13,sizeof(features13));
            if (!features13.UnrestrictedBufferTextureCopyPitchSupported)
            {
                _validGraphicsCard = false;
                return;
            }
            auto backend = static_cast<DX12Backend*>(Backend::current());

            if(backend->_dx12DebugHandler)
            {
                Microsoft::WRL::ComPtr<ID3D12InfoQueue> pInfoQueue;
                if (SUCCEEDED(_device.As(&pInfoQueue)))
                {
                    pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, FALSE);
                    pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, FALSE);
                    pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, FALSE);
                    pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_INFO,FALSE);
                    pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_MESSAGE,FALSE);



                    D3D12_INFO_QUEUE_FILTER NewFilter = {};
                    NewFilter.DenyList.NumSeverities = 0;
                    NewFilter.DenyList.pSeverityList = nullptr;
                    NewFilter.DenyList.NumIDs = 0;
                    NewFilter.DenyList.pIDList = nullptr;

                    pInfoQueue->PushStorageFilter(&NewFilter);
                }

                Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
                auto res = _device->QueryInterface(IID_PPV_ARGS(&infoQueue));
                Microsoft::WRL::ComPtr<ID3D12InfoQueue1> infoQueue1 = nullptr;
                infoQueue.As(&infoQueue1);
                DWORD callBackCookie = 0;
                infoQueue1->RegisterMessageCallback(DX12ErrorCallback,D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &callBackCookie);

            }

            _graphics = new DX12Queue(_device,GPUQueue::QueueType::GRAPHICS);
            _compute = new DX12Queue(_device, GPUQueue::QueueType::COMPUTE);
            _transfer = new DX12Queue(_device, GPUQueue::QueueType::TRANSFER);


            D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
            allocatorDesc.pDevice = _device.Get();
            allocatorDesc.pAdapter = adapter.Get();
            // These flags are optional but recommended.
            allocatorDesc.Flags = static_cast<D3D12MA::ALLOCATOR_FLAGS>(D3D12MA::ALLOCATOR_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED | D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED);
            D3D12MA::CreateAllocator(&allocatorDesc,&_allocator);

            D3D12MA::POOL_DESC poolDesc{};
            poolDesc.HeapProperties.Type = D3D12_HEAP_TYPE_CUSTOM;
            poolDesc.HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
            poolDesc.HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
            poolDesc.Flags = D3D12MA::POOL_FLAG_NONE;
            poolDesc.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
            _allocator->CreatePool(&poolDesc,&_sharedMemoryPool);

            D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
            samplerHeapDesc.NumDescriptors = 2048;
            samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
            samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            _device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&_samplerHeap));
            _validGraphicsCard = true;
        }

        uint64_t DX12GraphicsCard::videoMemory()
        {
            throw std::runtime_error("Not implemented");
        }

        std::string DX12GraphicsCard::name()
        {
            throw std::runtime_error("Not implemented");
        }

        GPUQueue* DX12GraphicsCard::graphicsQueue()
        {
            return _graphics;
        }

        GPUQueue* DX12GraphicsCard::computeQueue()
        {
            return _compute;
        }

        GPUQueue* DX12GraphicsCard::transferQueue()
        {
            return _transfer;
        }

        uint64_t DX12GraphicsCard::uniformBufferOffsetAlignment()
        {
            throw std::runtime_error("Not implemented");
        }

        uint64_t DX12GraphicsCard::storageBufferOffsetAlignment()
        {
            throw std::runtime_error("Not implemented");
        }

        void DX12GraphicsCard::defragmentMemory(SemaphoreValue* waitFor, size_t waitForCount, SemaphoreValue* signal,
            size_t signalCount)
        {
            throw std::runtime_error("Not implemented");
        }

        bool DX12GraphicsCard::isValidGraphicsCard() const
        {
            return _validGraphicsCard;
        }

        DX12GraphicsCard* DX12GraphicsCard::selected()
        {
            return static_cast<DX12GraphicsCard*>(slagGraphicsCard());
        }

        Microsoft::WRL::ComPtr<ID3D12Device2> DX12GraphicsCard::device() const
        {
            return _device;
        }

        ID3D12DescriptorHeap* DX12GraphicsCard::samplerHeap() const
        {
            return _samplerHeap;
        }

        D3D12MA::Allocator* DX12GraphicsCard::allocator() const
        {
            return _allocator;
        }

        D3D12MA::Pool* DX12GraphicsCard::sharedMemoryPool() const
        {
            return _sharedMemoryPool;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE DX12GraphicsCard::getSamplerHandle()
        {
            if(!_freedSamplerHandles.empty())
            {
                auto handle = _freedSamplerHandles.front();
                _freedSamplerHandles.pop();
                return handle;
            }

            auto h = _samplerHeap->GetCPUDescriptorHandleForHeapStart();
            h.ptr+=(_samplerIndex*_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER));
            auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(_samplerHeap->GetCPUDescriptorHandleForHeapStart(),_samplerIndex,_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)) ;//_samplerHeap->GetCPUDescriptorHandleForHeapStart().MakeOffsetted(_samplerIndex*_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER));
            _samplerIndex++;
            return handle;
        }

        void DX12GraphicsCard::freeSamplerHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle)
        {
            _freedSamplerHandles.push(handle);
        }
    } // dx12
} // slag
