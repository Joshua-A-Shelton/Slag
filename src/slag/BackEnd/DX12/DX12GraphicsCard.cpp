#include "DX12GraphicsCard.h"
#include "DX12Lib.h"

namespace slag
{
    namespace dx
    {
        DX12GraphicsCard::DX12GraphicsCard(const Microsoft::WRL::ComPtr<IDXGIAdapter4>& adapter)
        {
            D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_device));
#ifndef NDEBUG
            Microsoft::WRL::ComPtr<ID3D12InfoQueue> pInfoQueue;
            if (SUCCEEDED(_device.As(&pInfoQueue)))
            {
                pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
                pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
                pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

                // Suppress whole categories of messages
                //D3D12_MESSAGE_CATEGORY Categories[] = {};

                // Suppress messages based on their severity level
                D3D12_MESSAGE_SEVERITY Severities[] =
                        {
                                D3D12_MESSAGE_SEVERITY_INFO
                        };

                // Suppress individual messages by their ID
                D3D12_MESSAGE_ID DenyIds[] = {
                        D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
                        D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
                        D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
                };

                D3D12_INFO_QUEUE_FILTER NewFilter = {};
                //NewFilter.DenyList.NumCategories = _countof(Categories);
                //NewFilter.DenyList.pCategoryList = Categories;
                NewFilter.DenyList.NumSeverities = _countof(Severities);
                NewFilter.DenyList.pSeverityList = Severities;
                NewFilter.DenyList.NumIDs = _countof(DenyIds);
                NewFilter.DenyList.pIDList = DenyIds;

                pInfoQueue->PushStorageFilter(&NewFilter);
            }
#endif

            D3D12_COMMAND_QUEUE_DESC desc = {};
            desc.Type =     D3D12_COMMAND_LIST_TYPE_DIRECT;
            desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
            desc.Flags =    D3D12_COMMAND_QUEUE_FLAG_NONE;
            desc.NodeMask = 0;


            ID3D12CommandQueue* render = nullptr;
            _device->CreateCommandQueue(&desc, IID_PPV_ARGS(&render));
            _render = new DX12Queue(render,slag::GpuQueue::QueueType::Graphics);

            desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
            ID3D12CommandQueue* graphics = nullptr;
            _device->CreateCommandQueue(&desc, IID_PPV_ARGS(&graphics));
            _graphics = new DX12Queue(graphics,slag::GpuQueue::QueueType::Graphics);

            desc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
            ID3D12CommandQueue* transfer = nullptr;
            _device->CreateCommandQueue(&desc, IID_PPV_ARGS(&transfer));
            _graphics = new DX12Queue(transfer,slag::GpuQueue::QueueType::Transfer);

            desc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
            ID3D12CommandQueue* compute = nullptr;
            _device->CreateCommandQueue(&desc, IID_PPV_ARGS(&compute));
            _graphics = new DX12Queue(compute,slag::GpuQueue::QueueType::Compute);

        }

        DX12GraphicsCard::~DX12GraphicsCard()
        {
            if(_render)
            {
                delete _render;
            }
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
        }

        Microsoft::WRL::ComPtr<ID3D12Device2>& DX12GraphicsCard::device()
        {
            return _device;
        }

        GpuQueue* DX12GraphicsCard::RenderQueue()
        {
            return _render;
        }

        GpuQueue* DX12GraphicsCard::GraphicsQueue()
        {
            return _graphics;
        }

        GpuQueue* DX12GraphicsCard::TransferQueue()
        {
            return _transfer;
        }

        GpuQueue* DX12GraphicsCard::ComputeQueue()
        {
            return _compute;
        }

    } // dx
} // slag