#include <iostream>
#include <cassert>
#include <comdef.h>
#include "DX12GraphicsCard.h"
#include "DX12Lib.h"
#include "directx/d3dx12.h"

namespace slag
{
    namespace dx
    {
        void (*SLAG_DX12_DEBUG_CALLBACK)(std::string& message, SlagInitDetails::DebugLevel level, int32_t messageID);

        void DX12ErrorCallback(D3D12_MESSAGE_CATEGORY category,D3D12_MESSAGE_SEVERITY severity,D3D12_MESSAGE_ID id, LPCSTR pdescription, void* pcontext)
        {
            if(SLAG_DX12_DEBUG_CALLBACK!=nullptr)
            {
                SlagInitDetails::DebugLevel level = SlagInitDetails::SLAG_MESSAGE;
                if(severity == D3D12_MESSAGE_SEVERITY_ERROR || severity == D3D12_MESSAGE_SEVERITY_CORRUPTION)
                {
                    level = SlagInitDetails::SLAG_ERROR;
                }
                else if(severity == D3D12_MESSAGE_SEVERITY_WARNING)
                {
                    level = SlagInitDetails::SLAG_WARNING;
                }
                std::string message = pdescription;
                SLAG_DX12_DEBUG_CALLBACK(message,level,id);
            }
        }

        DX12GraphicsCard::DX12GraphicsCard(const Microsoft::WRL::ComPtr<IDXGIAdapter4>& adapter, Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory, const SlagInitDetails& details)
        {
            _dxgiFactory = dxgiFactory;
            D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&_device));
            D3D12_FEATURE_DATA_D3D12_OPTIONS12 features{};
            auto res = _device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12,&features,sizeof(features));
            _supportsEnhancedBarriers = features.EnhancedBarriersSupported;
            D3D12_FEATURE_DATA_D3D12_OPTIONS13 features13{};
            res = _device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS13,&features13,sizeof(features13));
            assert(features13.UnrestrictedBufferTextureCopyPitchSupported && "DX12 renderer must support arbitrary texture copy pitch");
            /*D3D12_FEATURE_DATA_D3D12_OPTIONS16 features16{};
            res = _device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS16,&features16,sizeof(features16));
            _com_error err(res);
            LPCTSTR errMsg = err.ErrorMessage();
            //assert(features16.GPUUploadHeapSupported && "DX12 doesn't support gpu upload heaps");*/

            if(details.debug)
            {
                Microsoft::WRL::ComPtr<ID3D12InfoQueue> pInfoQueue;
                if (SUCCEEDED(_device.As(&pInfoQueue)))
                {
                    pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
                    pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
                    pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
                    pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_INFO,TRUE);
                    pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_MESSAGE,TRUE);

                    // Suppress whole categories of messages
                    //D3D12_MESSAGE_CATEGORY Categories[] = {};

                    // Suppress messages based on their severity level
                    D3D12_MESSAGE_SEVERITY Severities[] =
                            {
                                    D3D12_MESSAGE_SEVERITY_INFO
                            };

                    // Suppress individual messages by their ID
                    /*D3D12_MESSAGE_ID DenyIds[] = {
                            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
                            //D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
                            //D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
                    };*/

                    D3D12_INFO_QUEUE_FILTER NewFilter = {};
                    //NewFilter.DenyList.NumCategories = _countof(Categories);
                    //NewFilter.DenyList.pCategoryList = Categories;
                    NewFilter.DenyList.NumSeverities = 0;
                    NewFilter.DenyList.pSeverityList = nullptr;
                    NewFilter.DenyList.NumIDs = 0;
                    NewFilter.DenyList.pIDList = nullptr;

                    pInfoQueue->PushStorageFilter(&NewFilter);
                    SLAG_DX12_DEBUG_CALLBACK = details.slagDebugHandler;
                }

                Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
                auto res = _device->QueryInterface(IID_PPV_ARGS(&infoQueue));
                Microsoft::WRL::ComPtr<ID3D12InfoQueue1> infoQueue1 = nullptr;
                infoQueue.As(&infoQueue1);
                DWORD callBackCookie = 0;
                infoQueue1->RegisterMessageCallback(DX12ErrorCallback,D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &callBackCookie);

            }

            D3D12_COMMAND_QUEUE_DESC desc = {};
            desc.Type =     D3D12_COMMAND_LIST_TYPE_DIRECT;
            desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
            desc.Flags =    D3D12_COMMAND_QUEUE_FLAG_NONE;
            desc.NodeMask = 0;


            ID3D12CommandQueue* graphics = nullptr;
            _device->CreateCommandQueue(&desc, IID_PPV_ARGS(&graphics));
            _graphics = new DX12Queue(graphics,slag::GpuQueue::QueueType::Graphics);

            desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
            desc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
            ID3D12CommandQueue* transfer = nullptr;
            _device->CreateCommandQueue(&desc, IID_PPV_ARGS(&transfer));
            _transfer = new DX12Queue(transfer, slag::GpuQueue::QueueType::Transfer);

            desc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
            ID3D12CommandQueue* compute = nullptr;
            _device->CreateCommandQueue(&desc, IID_PPV_ARGS(&compute));
            _compute = new DX12Queue(compute, slag::GpuQueue::QueueType::Compute);


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
        }

        DX12GraphicsCard::~DX12GraphicsCard()
        {
            if(_graphics)
            {
                delete _graphics;
            }
            if(_transfer)
            {
                delete _transfer;
            }
            if(_compute)
            {
                delete _compute;
            }
            if(_sharedMemoryPool)
            {
                _sharedMemoryPool->Release();
            }
            if(_allocator)
            {
                _allocator->Release();
            }
            if(_samplerHeap)
            {
                _samplerHeap->Release();
            }
            _dxgiFactory->Release();
            //_device->Release();
        }

        ID3D12Device2* DX12GraphicsCard::device()
        {
            return _device.Get();
        }

        IDXGIFactory4* DX12GraphicsCard::dxgiFactory()
        {
            return _dxgiFactory.Get();
        }


        GpuQueue* DX12GraphicsCard::graphicsQueue()
        {
            return _graphics;
        }

        GpuQueue* DX12GraphicsCard::transferQueue()
        {
            return _transfer;
        }

        GpuQueue* DX12GraphicsCard::computeQueue()
        {
            return _compute;
        }

        D3D12MA::Allocator* DX12GraphicsCard::allocator()
        {
            return _allocator;
        }

        ID3D12DescriptorHeap* DX12GraphicsCard::samplerHeap()
        {
            return _samplerHeap;
        }

        void DX12GraphicsCard::defragmentMemory()
        {
            throw std::runtime_error("DX12GraphicsCard::defragmentMemory not implemented");
        }

        bool DX12GraphicsCard::supportsEnhancedBarriers()
        {
            return _supportsEnhancedBarriers;
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

        D3D12MA::Pool* DX12GraphicsCard::sharedMemoryPool()
        {
            return _sharedMemoryPool;
        }
    } // dx
} // slag