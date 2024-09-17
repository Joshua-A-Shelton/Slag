#include <iostream>
#include "DX12GraphicsCard.h"
#include "DX12Lib.h"

namespace slag
{
    namespace dx
    {
        void DX12ErrorCallback(D3D12_MESSAGE_CATEGORY category,D3D12_MESSAGE_SEVERITY severity,D3D12_MESSAGE_ID id, LPCSTR pdescription, void* pcontext)
        {
            std::cout << pdescription << std::endl;
        }

        DX12GraphicsCard::DX12GraphicsCard(const Microsoft::WRL::ComPtr<IDXGIAdapter4>& adapter, Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory)
        {
            _dxgiFactory = dxgiFactory;
            D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_device));
#ifndef NDEBUG
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
            }

            Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
            auto res = _device->QueryInterface(IID_PPV_ARGS(&infoQueue));
            Microsoft::WRL::ComPtr<ID3D12InfoQueue1> infoQueue1 = nullptr;
            infoQueue.As(&infoQueue1);
            DWORD callBackCookie = 0;
            infoQueue1->RegisterMessageCallback(DX12ErrorCallback,D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &callBackCookie);

#endif

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
            _transfer = new DX12Queue(transfer,slag::GpuQueue::QueueType::Transfer);

            desc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
            ID3D12CommandQueue* compute = nullptr;
            _device->CreateCommandQueue(&desc, IID_PPV_ARGS(&compute));
            _compute = new DX12Queue(compute,slag::GpuQueue::QueueType::Compute);


            D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
            allocatorDesc.pDevice = _device.Get();
            allocatorDesc.pAdapter = adapter.Get();
            // These flags are optional but recommended.
            allocatorDesc.Flags = static_cast<D3D12MA::ALLOCATOR_FLAGS>(D3D12MA::ALLOCATOR_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED | D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED);
            D3D12MA::CreateAllocator(&allocatorDesc,&_allocator);

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
            if(_allocator)
            {
                _allocator->Release();
            }
            //_dxgiFactory->Release();
            //_device->Release();
        }

        Microsoft::WRL::ComPtr<ID3D12Device2>& DX12GraphicsCard::device()
        {
            return _device;
        }

        Microsoft::WRL::ComPtr<IDXGIFactory4>& DX12GraphicsCard::dxgiFactory()
        {
            return _dxgiFactory;
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

        void DX12GraphicsCard::defragmentMemory()
        {
            throw std::runtime_error("not implemented");
        }
    } // dx
} // slag