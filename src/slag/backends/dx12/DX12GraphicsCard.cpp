#include "DX12GraphicsCard.h"

#include "DX12Backend.h"
#include "DX12Buffer.h"
#include "DX12CommandBuffer.h"
#include "DX12Semaphore.h"
#include "DX12ShaderModule.h"
#include "DX12SubmissionQueue.h"
#include "DX12Texture.h"
#include "slag/exceptions/NotImplemented.h"
#undef ERROR
#undef max

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

            _graphicsQueue = new DX12SubmissionQueue(this,QueueType::GRAPHICS);
            _computeQueue = new DX12SubmissionQueue(this,QueueType::COMPUTE);
            _transferQueue = new DX12SubmissionQueue(this,QueueType::TRANSFER);

            D3D12MA::POOL_DESC poolDesc{};
            poolDesc.HeapProperties.Type = D3D12_HEAP_TYPE_CUSTOM;
            poolDesc.HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
            poolDesc.HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
            poolDesc.Flags = D3D12MA::POOL_FLAG_NONE;
            poolDesc.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
            _allocator->CreatePool(&poolDesc,&_cpuReadablePool);

            //Memory Properties
            _memoryProperties.videoMemory = desc.DedicatedVideoMemory;
            _memoryProperties.cacheCoherentSharedMemory = architecture.CacheCoherentUMA;
            _memoryProperties.maxUniformBufferSize = 65536;

            //capabilities
            _capabilities.defragmentable = true;
            D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
            if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5))) && options5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0)
            {
                _capabilities.raytracing = true;
            }

            //descriptor heap details
            D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
            _device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options));
            _descriptorHeapDetails.resourceDescriptorIncrementSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            _descriptorHeapDetails.samplerDescriptorIncrementSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
            _descriptorHeapDetails.maxResourceDescriptorHeapSize = 1015808 *  _descriptorHeapDetails.resourceDescriptorIncrementSize;
            _descriptorHeapDetails.maxSamplerDescriptorHeapSize = 4000 * _descriptorHeapDetails.samplerDescriptorIncrementSize;
            _descriptorHeapDetails.resourceReservedRangeSize = 0;
            _descriptorHeapDetails.samplerReservedRangeSize = 0;

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

        DX12GraphicsCard::DX12GraphicsCard(DX12GraphicsCard&& from) noexcept
        {
            move(from);
        }

        DX12GraphicsCard& DX12GraphicsCard::operator=(DX12GraphicsCard&& from) noexcept
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

        const GraphicsCardMemoryProperties& DX12GraphicsCard::memoryProperties() const
        {
            return _memoryProperties;
        }

        const GraphicsCardCapabilities& DX12GraphicsCard::capabilities() const
        {
            return _capabilities;
        }

        const DescriptorHeapDetails& DX12GraphicsCard::descriptorHeapDetails() const
        {
            return _descriptorHeapDetails;
        }

        PixelFormatProperties DX12GraphicsCard::formatProperties(PixelFormat format) const
        {
             PixelFormatProperties properties{};
            if (format == PixelFormat::UNDEFINED)
            {
                properties.tiling = TextureTiling::UNSUPPORTED;
                return properties;
            }

            auto dxformat = DX12Backend::nativeFormat(format);
            D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport{dxformat};
            _device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_INFO,&formatSupport,sizeof(formatSupport));
            if (formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_NONE )
            {
                return properties;
            }

            if (formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE)
            {
                properties.validUsageFlags |= TextureUsageFlags::SAMPLED;
            }

            if (formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_RENDER_TARGET)
            {
                properties.validUsageFlags |= TextureUsageFlags::COLOR_TARGET;
            }
            if (formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL)
            {
                properties.validUsageFlags |= TextureUsageFlags::DEPTH_STENCIL_TARGET;
            }
            if (formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW)
            {
                properties.validUsageFlags |= TextureUsageFlags::UNORDERED_ACCESS;
            }

            //TODO: not sure if this is right or not.... but there's not great documentation on which allow linear or not
            //https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/checking-hardware-feature-support
            if (formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE)
            {
                properties.linearFilteringCapable = true;
            }
            else
            {
                properties.linearFilteringCapable = false;
            }

            if (formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE)
            {
                properties.blitSource = true;
            }
            if (formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_RENDER_TARGET)
            {
                properties.blitDestination = true;
            }

            //TODO: not sure about this... I think it's right, but could easily be wrong
            if (formatSupport.Support2 & D3D12_FORMAT_SUPPORT2_TILED)
            {
                properties.tiling = TextureTiling::OPTIMIZED;
            }
            else
            {
                properties.tiling = TextureTiling::LINEAR;
            }

            return properties;
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
            uint64_t targetBytes,
            std::function<void(MemoryReference*)> memoryMoved)
        {
            D3D12MA::DEFRAGMENTATION_DESC defragDesc = {};
            defragDesc.Flags = D3D12MA::DEFRAGMENTATION_FLAG_ALGORITHM_BALANCED;
            defragDesc.MaxBytesPerPass = targetBytes;

            D3D12MA::DefragmentationContext* defragCtx;
            _allocator->BeginDefragmentation(&defragDesc, &defragCtx);

            for(;;)
            {
                D3D12MA::DEFRAGMENTATION_PASS_MOVE_INFO pass;
                HRESULT hr = defragCtx->BeginPass(&pass);
                if(hr == S_OK)
                {
                    break;
                }
                else if(hr != S_FALSE)
                {
                    throw std::runtime_error("failed to begin defragmentation");
                }

                std::vector<ID3D12Resource*> movedResources;
                std::vector<MemoryReference*> movedMemoryRefs;
                DX12CommandBuffer moveCB(this,QueueType::GRAPHICS);
                std::vector<D3D12_TEXTURE_BARRIER> textureBarriers;
                DX12Semaphore moved(this,0);
                moveCB.begin();
                for(UINT i = 0; i < pass.MoveCount; ++i)
                {
                    MemoryReference* userData = (MemoryReference*)pass.pMoves[i].pSrcAllocation->GetPrivateData();

                    if (userData->type == MemoryObjectType::TEXTURE)
                    {
                        auto texture = static_cast<DX12Texture*>(userData->memory.texture);
                        auto movedTexture = texture->moveMemory(pass.pMoves[i].pDstTmpAllocation,&moveCB,textureBarriers);
                        movedResources.push_back(movedTexture);
                        movedMemoryRefs.push_back(userData);

                    }
                    else
                    {
                        auto buffer = static_cast<DX12Buffer*>(userData->memory.buffer);
                        auto movedBuffer = buffer->moveMemory(pass.pMoves[i].pDstTmpAllocation,&moveCB);
                        movedResources.push_back(movedBuffer);
                        movedMemoryRefs.push_back(userData);
                    }
                }
                D3D12_BARRIER_GROUP barrierGroup =
                {
                    .Type = D3D12_BARRIER_TYPE_TEXTURE,
                    .NumBarriers = static_cast<uint32_t>(textureBarriers.size()),
                    .pTextureBarriers = textureBarriers.data()
                };

                moveCB.dx12Handle()->Barrier(1,&barrierGroup);

                moveCB.end();
                CommandBuffer* movedPtr = &moveCB;
                SemaphoreValue movedValue{.semaphore = &moved,.value = 1};
                SubmissionBatch batch
                {
                    .waitSemaphores = nullptr,
                    .waitSemaphoreCount = 0,
                    .commandBuffers = &movedPtr,
                    .commandBufferCount = 1,
                    .signalSemaphores = &movedValue,
                    .signalSemaphoreCount = 1,
                };

                _graphicsQueue->submit(&batch,1);
                moved.waitForValue(1);
                for (auto i=0; i< movedResources.size(); i++)
                {
                    movedResources[i]->Release();
                }
                movedResources.clear();
                for (auto i=0; i< movedMemoryRefs.size(); i++)
                {
                    if (movedMemoryRefs[i]->type == MemoryObjectType::BUFFER)
                    {
                        static_cast<DX12Buffer*>(movedMemoryRefs[i]->memory.buffer)->updatePointer();
                    }
                    if (memoryMoved != nullptr)
                    {
                        memoryMoved(movedMemoryRefs[i]);
                    }
                }

                textureBarriers.clear();
                movedMemoryRefs.clear();

                hr = defragCtx->EndPass(&pass);
                if(hr == S_OK)
                {
                    break;
                }
                else if(hr != S_FALSE)
                {
                    throw std::runtime_error("failed to defragment graphics memory");
                }

            }
            D3D12MA::DEFRAGMENTATION_STATS stats;
            defragCtx->GetStats(&stats);
            defragCtx->Release();
            return stats.BytesMoved;
        }

        ShaderModule* DX12GraphicsCard::newShaderModule(ShaderLanguage language, void* data, uint32_t dataLength)
        {
            return new DX12ShaderModule(this,language,data,dataLength);
        }

        ShaderPipeline* DX12GraphicsCard::newShaderPipeline(
            const VertexDescription& vertexDescription,
            ShaderModule* vertexShader,
            ShaderModule* fragmentShader,
            PipelineInputMapping* inputBindings,
            const PipelineState& pipelineState,
            const FramebufferDescription& framebufferDescription)
        {
            throw NotImplemented();
        }

        CommandBuffer* DX12GraphicsCard::newCommandBuffer(QueueType type)
        {
            return new DX12CommandBuffer(this,type);
        }

        Semaphore* DX12GraphicsCard::newSemaphore(uint64_t initialValue)
        {
            return new DX12Semaphore(this,initialValue);
        }

        Buffer* DX12GraphicsCard::newBuffer(
            uint64_t size,
            BufferCPUAccess cpuAccess,
            BufferMemoryType memoryType)
        {
            return new DX12Buffer(this,size,cpuAccess,memoryType);
        }

        DescriptorHeap* DX12GraphicsCard::newDescriptorHeap(DescriptorHeapType type, uint32_t descriptorCount)
        {
            throw NotImplemented();
        }

        Texture* DX12GraphicsCard::newTexture1D(uint32_t width, PixelFormat format, TextureUsageFlags usage, uint32_t mipLevels,uint32_t arrayDepth)
        {
            return new DX12Texture(this,width,format,usage,mipLevels,arrayDepth);
        }

        Texture* DX12GraphicsCard::newTexture2D(uint32_t width, uint32_t height, PixelFormat format, TextureUsageFlags usage, uint32_t mipLevels,
            SampleCount sampleCount, uint32_t layers)
        {
            return new DX12Texture(this,width,height,format,usage,mipLevels,sampleCount,layers);
        }

        Texture* DX12GraphicsCard::newTexture3D(uint32_t width, uint32_t height, uint32_t depth, PixelFormat format, TextureUsageFlags usage,
            uint32_t mipLevels)
        {
            return new DX12Texture(this,width,height,depth,format,usage,mipLevels);
        }

        Texture* DX12GraphicsCard::newTextureCube(uint32_t dimension, PixelFormat format, TextureUsageFlags usage, uint32_t mipLevels,uint32_t arrayDepth)
        {
            return new DX12Texture(this,format,usage,dimension,mipLevels,arrayDepth);
        }

        Sampler* DX12GraphicsCard::newSampler(SamplerFilter min, SamplerFilter mag, SamplerFilter mip,
            SamplerAddressMode u, SamplerAddressMode v, SamplerAddressMode w, float mipLODBias, bool anisotrophyEnabled,
            uint8_t maxAnisotrophy, ComparisonFunction comparisonFunction, Color borderColor, float minLOD,
            float maxLOD)
        {
            throw NotImplemented();
        }

        D3D12MA::Allocator* DX12GraphicsCard::allocator() const
        {
            return _allocator;
        }

        D3D12MA::Pool* DX12GraphicsCard::cpuReadablePool() const
        {
            return _cpuReadablePool;
        }

        Microsoft::WRL::ComPtr<ID3D12Device2>& DX12GraphicsCard::device()
        {
            return _device;
        }

        void DX12GraphicsCard::move(DX12GraphicsCard& from)
        {
            _memoryProperties = from._memoryProperties;
            _capabilities = from._capabilities;
            _device = from._device;
            _descriptorHeapDetails = from._descriptorHeapDetails;
            _dxgiFactory = from._dxgiFactory;
            _dxgiAdapter4 = from._dxgiAdapter4;
            std::swap(_allocator, from._allocator);
            std::swap(_graphicsQueue,from._graphicsQueue);
            std::swap(_computeQueue,from._computeQueue);
            std::swap(_transferQueue,from._transferQueue);
        }
    } // dx12
} // slag
