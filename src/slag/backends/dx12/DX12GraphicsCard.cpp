#include "DX12GraphicsCard.h"

#include "DX12Backend.h"
#include "DX12Buffer.h"
#include "DX12CommandBuffer.h"
#include "DX12Semaphore.h"
#include "DX12SubmissionQueue.h"
#include "slag/exceptions/NotImplemented.h"
#undef ERROR

namespace slag
{
    namespace dx12
    {
        void DX12ErrorCallback(D3D12_MESSAGE_CATEGORY category,D3D12_MESSAGE_SEVERITY severity,D3D12_MESSAGE_ID id, LPCSTR pdescription, void* pcontext)
        {
            auto backend = static_cast<DX12Backend*>(slag::Slag::backend());
            auto handler = backend->getDebugHandler();
            if(handler!=nullptr)
            {
                DebugLevel level = DebugLevel::INFO;
                if(severity == D3D12_MESSAGE_SEVERITY_ERROR || severity == D3D12_MESSAGE_SEVERITY_CORRUPTION)
                {

                    level = DebugLevel::ERROR;
                }
                else if(severity == D3D12_MESSAGE_SEVERITY_WARNING)
                {
                    level = DebugLevel::WARNING;
                }
                std::string message = pdescription;
                handler(message,level,id);
            }
        }

        DX12GraphicsCard::DX12GraphicsCard(
            Microsoft::WRL::ComPtr<ID3D12Device2> device,
            Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory,
            Microsoft::WRL::ComPtr<IDXGIAdapter4> dxgiAdapter,
            bool includeDebugHandling)
        {
            _device = device;
            _dxgiFactory = dxgiFactory;
            _dxgiAdapter4 = dxgiAdapter;

            if (includeDebugHandling)
            {
                Microsoft::WRL::ComPtr<ID3D12InfoQueue> pInfoQueue;
                if (SUCCEEDED(_device.As(&pInfoQueue)))
                {
                    pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, FALSE);
                    pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, FALSE);
                    pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, FALSE);
                    pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_INFO,FALSE);
                    pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_MESSAGE,FALSE);

                    D3D12_MESSAGE_CATEGORY hide[] =
                    {
                        D3D12_MESSAGE_CATEGORY_STATE_CREATION,
                    };

                    D3D12_INFO_QUEUE_FILTER NewFilter = {};
                    NewFilter.DenyList.NumSeverities = 0;
                    NewFilter.DenyList.pSeverityList = nullptr;
                    NewFilter.DenyList.NumCategories = std::size(hide);
                    NewFilter.DenyList.pCategoryList = hide;

                    pInfoQueue->PushStorageFilter(&NewFilter);
                }

                Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
                auto res = _device->QueryInterface(IID_PPV_ARGS(&infoQueue));
                Microsoft::WRL::ComPtr<ID3D12InfoQueue1> infoQueue1 = nullptr;
                infoQueue.As(&infoQueue1);
                DWORD callBackCookie = 0;
                infoQueue1->RegisterMessageCallback(DX12ErrorCallback,D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &callBackCookie);
            }


            D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
            allocatorDesc.pDevice = _device.Get();
            allocatorDesc.pAdapter = _dxgiAdapter4.Get();
            // These flags are optional but recommended.
            allocatorDesc.Flags = static_cast<D3D12MA::ALLOCATOR_FLAGS>(D3D12MA::ALLOCATOR_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED | D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED);
            D3D12MA::CreateAllocator(&allocatorDesc,&_allocator);

            DXGI_ADAPTER_DESC3 desc;
            _dxgiAdapter4->GetDesc3(&desc);
            D3D12_FEATURE_DATA_ARCHITECTURE1 architecture{};
            _device->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE1,&architecture,sizeof(D3D12_FEATURE_DATA_ARCHITECTURE1));

            _videoMemory = desc.DedicatedVideoMemory;
            _sharedMemory = architecture.CacheCoherentUMA;

            _graphicsQueue = new DX12SubmissionQueue(this,QueueType::GRAPHICS);
            _computeQueue = new DX12SubmissionQueue(this,QueueType::COMPUTE);
            _transferQueue = new DX12SubmissionQueue(this,QueueType::TRANSFER);
        }

        DX12GraphicsCard::~DX12GraphicsCard()
        {
            if (_graphicsQueue)
            {
                delete _graphicsQueue;
                delete _computeQueue;
                delete _transferQueue;
            }
        }

        DX12GraphicsCard::DX12GraphicsCard(DX12GraphicsCard&& from)
        {
            move(from);
        }

        DX12GraphicsCard& DX12GraphicsCard::operator=(DX12GraphicsCard&& from)
        {
            move(from);
            return *this;
        }

        std::string DX12GraphicsCard::name() const
        {
            std::string s_out;

            DXGI_ADAPTER_DESC1 desc;
            _dxgiAdapter4->GetDesc1(&desc);
            size_t len = std::wcstombs(nullptr, desc.Description, 0);
            if (len != static_cast<size_t>(-1))
            {
                // Allocate space in the std::string
                s_out.resize(len);
                // Perform the actual conversion
                std::wcstombs(&s_out[0], desc.Description, len);
            }
            return s_out;
        }

        uint64_t DX12GraphicsCard::videoMemory() const
        {
            return _videoMemory;
        }

        uint64_t DX12GraphicsCard::maxShaderAccessReadOnlyBufferSize() const
        {
            return 65536;
        }

        bool DX12GraphicsCard::cacheCoherentSharedMemory() const
        {
            return _sharedMemory;
        }

        SubmissionQueue* DX12GraphicsCard::graphicsQueue()
        {
            return _graphicsQueue;
        }

        SubmissionQueue* DX12GraphicsCard::computeQueue()
        {
            return _computeQueue;
        }

        SubmissionQueue* DX12GraphicsCard::transferQueue()
        {
            return _transferQueue;
        }

        uint64_t DX12GraphicsCard::defragmentMemory(
            SemaphoreValue* waitFor,
            uint32_t waitCount,
            SemaphoreValue* signal,
            uint32_t signalCount,
            uint64_t targetBytes)
        {
            throw NotImplemented();
        }

        CommandBuffer* DX12GraphicsCard::newCommandBuffer(QueueType type)
        {
            return new DX12CommandBuffer(this,type,CommandBufferLevel::PRIMARY);
        }

        Semaphore* DX12GraphicsCard::newSemaphore(uint64_t initialValue)
        {
            return new DX12Semaphore(this,initialValue);
        }

        Buffer* DX12GraphicsCard::newBuffer(
            uint64_t size,
            BufferUsage usage,
            BufferShaderAccess shaderAccess,
            BufferCPUAccess cpuAccess)
        {
            return new DX12Buffer(this,size,usage,shaderAccess,cpuAccess);
        }

        D3D12MA::Allocator* DX12GraphicsCard::allocator()
        {
            return _allocator;
        }

        Microsoft::WRL::ComPtr<ID3D12Device2>& DX12GraphicsCard::device()
        {
            return _device;
        }

        void DX12GraphicsCard::move(DX12GraphicsCard& from)
        {
            _device = from._device;
            _dxgiFactory = from._dxgiFactory;
            _dxgiAdapter4 = from._dxgiAdapter4;
            std::swap(_graphicsQueue,from._graphicsQueue);
            std::swap(_computeQueue,from._computeQueue);
            std::swap(_transferQueue,from._transferQueue);
        }
    } // dx12
} // slag
