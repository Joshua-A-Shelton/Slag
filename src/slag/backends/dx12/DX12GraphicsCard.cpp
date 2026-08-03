#include "DX12GraphicsCard.h"

#include "DX12Backend.h"
#include "DX12Buffer.h"
#include "DX12CommandBuffer.h"
#include "DX12Semaphore.h"
#include "DX12ShaderPipeline.h"
#include "DX12SubmissionQueue.h"
#include "DX12ResourceDescriptorHeap.h"
#include "DX12Texture.h"
#include <directx/d3dx12.h>

#include "DX12Sampler.h"
#include "DX12SamplerDescriptorHeap.h"
#include "DX12SwapChain.h"
#include "slag/exceptions/NotImplemented.h"
#include "slag/utilities/SLAG_ASSERT.h"
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
            auto result = _allocator->CreatePool(&poolDesc,&_cpuReadablePool);
            if (FAILED(result))
            {
                throw std::runtime_error("Failed to create CPU readable buffer pool");
            }

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
            _descriptorHeapDetails.textureDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            _descriptorHeapDetails.textureDescriptorAlignment = _descriptorHeapDetails.textureDescriptorSize;
            _descriptorHeapDetails.bufferDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            _descriptorHeapDetails.bufferDescriptorAlignment = _descriptorHeapDetails.bufferDescriptorSize;
            _descriptorHeapDetails.samplerDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
            _descriptorHeapDetails.samplerDescriptorAlignment = _descriptorHeapDetails.samplerDescriptorSize;

            if (options.ResourceHeapTier == D3D12_RESOURCE_HEAP_TIER_1)
            {
                _descriptorHeapDetails.maxResourceDescriptors = 1000000;
                _descriptorHeapDetails.maxSamplerDescriptors = 16;
                _descriptorHeapDetails.resourceReservedRangeSize = 0;
                _descriptorHeapDetails.samplerReservedRangeSize = 0;
            }
            else if (options.ResourceHeapTier == D3D12_RESOURCE_HEAP_TIER_2)
            {
                _descriptorHeapDetails.maxResourceDescriptors = 1000000;
                _descriptorHeapDetails.maxSamplerDescriptors = 2048;
                _descriptorHeapDetails.resourceReservedRangeSize = 0;
                _descriptorHeapDetails.samplerReservedRangeSize = 0;
            }
            else
            {
                _descriptorHeapDetails.maxResourceDescriptors = 1000000;
                _descriptorHeapDetails.maxSamplerDescriptors = 2048;
                _descriptorHeapDetails.resourceReservedRangeSize = 0;
                _descriptorHeapDetails.samplerReservedRangeSize = 0;
            }

            //root signature
            D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED |
            D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED |
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

            D3D12_ROOT_PARAMETER rootParameter
            {
                .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
                .Constants = {.ShaderRegister = 0, .RegisterSpace = 0, .Num32BitValues = 32},
                .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
            };

            D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
            rootSignatureDesc.NumParameters = 1;
            rootSignatureDesc.pParameters = &rootParameter;
            rootSignatureDesc.NumStaticSamplers = 0;
            rootSignatureDesc.pStaticSamplers = nullptr;
            rootSignatureDesc.Flags = rootSignatureFlags;

            // Serialize and create the signature
            Microsoft::WRL::ComPtr<ID3DBlob> signature;
            Microsoft::WRL::ComPtr<ID3DBlob> error;

            HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &signature, &error);
            if (FAILED(hr))
            {
                throw std::runtime_error(std::string("Failed to serialize root signature: ") + std::string(reinterpret_cast<const char*>(error->GetBufferPointer()), error->GetBufferSize()));
            }

            device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&_rootSignature));

        }

        DX12GraphicsCard::~DX12GraphicsCard()
        {
            if (_graphicsQueue)
            {
                delete _graphicsQueue;
                delete _computeQueue;
                delete _transferQueue;
                _cpuReadablePool->Release();
                _rootSignature->Release();
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
                properties.validUsageFlags |= TextureUsageFlags::READ_WRITE;
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

        SwapChain* DX12GraphicsCard::newSwapchain(const PlatformData& platformData, uint32_t width, uint32_t height, const SwapChainParameters& parameters)
        {
            return new DX12SwapChain(this,platformData,width,height,parameters);
        }

        ShaderPipeline* DX12GraphicsCard::newShaderPipeline(
            const VertexDescription& vertexDescription,
            const ShaderCode& vertexShader,
            const ShaderCode& fragmentShader,
            const PipelineState& pipelineState,
            const FramebufferDescription& framebufferDescription)
        {
            return new DX12ShaderPipeline(this,vertexDescription,vertexShader,fragmentShader,pipelineState,framebufferDescription);
        }

        ShaderPipeline* DX12GraphicsCard::newShaderPipeline(ShaderCode* computeShader)
        {
            return new DX12ShaderPipeline(this,computeShader);
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

        ResourceDescriptorHeap* DX12GraphicsCard::newResourceDescriptorHeap(uint32_t minDescriptorCount)
        {
            return new DX12ResourceDescriptorHeap(this,minDescriptorCount);
        }

        SamplerDescriptorHeap* DX12GraphicsCard::newSamplerDescriptorHeap(uint32_t descriptorCount)
        {
            return new DX12SamplerDescriptorHeap(this,descriptorCount);
        }

        void DX12GraphicsCard::writeUniformBufferDescriptor(Buffer* buffer, uint64_t offset, uint64_t length,
            void* destination)
        {
            SLAG_ASSERT(buffer->memoryType() == BufferMemoryType::UNIFORM && "Only uniform buffers can be bound for uniform buffer descriptors");
            auto dxBuffer = static_cast<DX12Buffer*>(buffer);
            D3D12_CPU_DESCRIPTOR_HANDLE handle((size_t)destination);

            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
            cbvDesc.BufferLocation = dxBuffer->deviceAddress() + offset;
            cbvDesc.SizeInBytes = length;

            _device->CreateConstantBufferView(&cbvDesc,handle);
        }

        void DX12GraphicsCard::writeReadWriteBufferDescriptor(Buffer* buffer, uint64_t firstElementIndex,
            uint64_t elementCount, uint64_t elementStride, void* destination)
        {
            auto dxBuffer = static_cast<DX12Buffer*>(buffer);
            D3D12_CPU_DESCRIPTOR_HANDLE handle((size_t)destination);
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
            uavDesc.Format = DXGI_FORMAT_UNKNOWN;
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            uavDesc.Buffer.FirstElement = firstElementIndex;
            uavDesc.Buffer.NumElements = elementCount;
            uavDesc.Buffer.StructureByteStride = elementStride;
            _device->CreateUnorderedAccessView(dxBuffer->dx12Handle(),nullptr,&uavDesc,handle);
        }

        void DX12GraphicsCard::writeUniformTexelBuffer(Buffer* buffer, PixelFormat format, uint64_t firstElementIndex,
                                                       uint64_t elementCount, void* destination)
        {
            SLAG_ASSERT(buffer->memoryType() == BufferMemoryType::UNIFORM && "Only uniform buffers can be bound for uniform buffer descriptors");
            SLAG_ASSERT(Pixel::aspectFlags(format) == PixelAspectFlags::COLOR_FLAG && "Only color pixel formats can be used for texel buffer descriptors");
            D3D12_CPU_DESCRIPTOR_HANDLE handle((size_t)destination);
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = DX12Backend::nativeFormat(format);
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            srvDesc.Buffer.FirstElement = firstElementIndex;
            srvDesc.Buffer.NumElements = elementCount;
            srvDesc.Buffer.StructureByteStride = 0;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        }

        void DX12GraphicsCard::writeReadWriteTexelBuffer(Buffer* buffer, PixelFormat format, uint64_t firstElementIndex,
                                                         uint64_t elementCount, void* destination)
        {
            SLAG_ASSERT(Pixel::aspectFlags(format) == PixelAspectFlags::COLOR_FLAG && "Only color pixel formats can be used for texel buffer descriptors");
            auto dxBuffer = static_cast<DX12Buffer*>(buffer);
            D3D12_CPU_DESCRIPTOR_HANDLE handle((size_t)destination);
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
            uavDesc.Format = DX12Backend::nativeFormat(format);
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            uavDesc.Buffer.FirstElement = firstElementIndex;
            uavDesc.Buffer.NumElements = elementCount;
            uavDesc.Buffer.StructureByteStride = 0;
            _device->CreateUnorderedAccessView(dxBuffer->dx12Handle(),nullptr,&uavDesc,handle);
        }

        void DX12GraphicsCard::writeUniformTextureDescriptor(Texture* texture, uint32_t baseMip, uint32_t mipCount,
            uint32_t baseLayer, uint32_t layerCount, void* destination)
        {
            auto dxTexture = static_cast<DX12Texture*>(texture);
            D3D12_CPU_DESCRIPTOR_HANDLE handle((size_t)destination);
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = DX12Backend::nativeFormat(dxTexture->format());
            srvDesc.ViewDimension = DX12Backend::nativeSRVTextureDimension(dxTexture->type(),dxTexture->layers(),dxTexture->sampleCount());
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            switch (srvDesc.ViewDimension)
            {
            case D3D12_SRV_DIMENSION_TEXTURE1D:
                srvDesc.Texture1D.MostDetailedMip = baseMip;
                srvDesc.Texture1D.MipLevels = mipCount;
                srvDesc.Texture1D.ResourceMinLODClamp = 0;
                break;
            case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
                srvDesc.Texture1DArray.MostDetailedMip = baseMip;
                srvDesc.Texture1DArray.MipLevels = mipCount;
                srvDesc.Texture1DArray.ResourceMinLODClamp = 0;
                srvDesc.Texture1DArray.ArraySize = layerCount;
                srvDesc.Texture1DArray.FirstArraySlice = baseLayer;
                break;
            case D3D12_SRV_DIMENSION_TEXTURE2D:
                srvDesc.Texture2D.MostDetailedMip = baseMip;
                srvDesc.Texture2D.MipLevels = mipCount;
                srvDesc.Texture2D.ResourceMinLODClamp = 0;
                srvDesc.Texture2D.PlaneSlice = 0;
                break;
            case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
                srvDesc.Texture2DArray.MostDetailedMip = baseMip;
                srvDesc.Texture2DArray.MipLevels = mipCount;
                srvDesc.Texture2DArray.ResourceMinLODClamp = 0;
                srvDesc.Texture2DArray.ArraySize = layerCount;
                srvDesc.Texture2DArray.FirstArraySlice = baseLayer;
                break;
            case D3D12_SRV_DIMENSION_TEXTURE2DMS:
                break;
            case D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY:
                srvDesc.Texture2DMSArray.ArraySize = layerCount;
                srvDesc.Texture2DMSArray.FirstArraySlice = baseLayer;
                break;
            case D3D12_SRV_DIMENSION_TEXTURE3D:
                srvDesc.Texture3D.MostDetailedMip = baseMip;
                srvDesc.Texture3D.MipLevels = mipCount;
                srvDesc.Texture3D.ResourceMinLODClamp = 0;
                break;
            case D3D12_SRV_DIMENSION_TEXTURECUBE:
                srvDesc.TextureCube.MostDetailedMip = baseMip;
                srvDesc.TextureCube.MipLevels = mipCount;
                srvDesc.TextureCube.ResourceMinLODClamp = 0;
                break;
            case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY:
                srvDesc.TextureCubeArray.MostDetailedMip = baseMip;
                srvDesc.TextureCubeArray.MipLevels = mipCount;
                srvDesc.TextureCubeArray.ResourceMinLODClamp = 0;
                srvDesc.TextureCubeArray.NumCubes = layerCount/6;
                srvDesc.TextureCubeArray.First2DArrayFace = baseLayer;
                break;
            }
            _device->CreateShaderResourceView(dxTexture->dx12Handle(),&srvDesc,handle);
        }

        void DX12GraphicsCard::writeReadWriteTextureDescriptor(Texture* texture, uint32_t mip, uint32_t baseLayer,
            uint32_t layerCount, void* destination)
        {
            SLAG_ASSERT((bool)(texture->usage() & TextureUsageFlags::READ_WRITE) && "Only unordered access textures can be bound for read-write texture descriptors");
            SLAG_ASSERT(texture->type() != TextureType::CUBE_MAP && "Unordered access textures cannot be cube maps");
            auto dxTexture = static_cast<DX12Texture*>(texture);
            D3D12_CPU_DESCRIPTOR_HANDLE handle((size_t)destination);
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
            uavDesc.Format = DX12Backend::nativeFormat(dxTexture->format());
            uavDesc.ViewDimension = DX12Backend::nativeUAVTextureDimension(dxTexture->type(),dxTexture->layers(),dxTexture->sampleCount());
            switch (uavDesc.ViewDimension)
            {
            case D3D12_UAV_DIMENSION_TEXTURE1D:
                uavDesc.Texture1D.MipSlice = mip;
                break;
            case D3D12_UAV_DIMENSION_TEXTURE1DARRAY:
                uavDesc.Texture1DArray.MipSlice = mip;
                uavDesc.Texture1DArray.ArraySize = layerCount;
                uavDesc.Texture1DArray.FirstArraySlice = baseLayer;
                break;
            case D3D12_UAV_DIMENSION_TEXTURE2D:
                uavDesc.Texture2D.MipSlice = mip;
                uavDesc.Texture2D.PlaneSlice = 0;
                break;
            case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
                uavDesc.Texture2DArray.MipSlice = mip;
                uavDesc.Texture2DArray.ArraySize = layerCount;
                uavDesc.Texture2DArray.FirstArraySlice = baseLayer;
                break;
            case D3D12_UAV_DIMENSION_TEXTURE2DMS:
                break;
            case D3D12_UAV_DIMENSION_TEXTURE2DMSARRAY:
                uavDesc.Texture2DMSArray.ArraySize = layerCount;
                uavDesc.Texture2DMSArray.FirstArraySlice = baseLayer;
                break;
            case D3D12_UAV_DIMENSION_TEXTURE3D:
                uavDesc.Texture3D.MipSlice = mip;
                uavDesc.Texture3D.FirstWSlice = baseLayer;
                uavDesc.Texture3D.WSize = layerCount;
                break;
            }
            _device->CreateUnorderedAccessView(dxTexture->dx12Handle(),nullptr,&uavDesc,handle);
        }

        void DX12GraphicsCard::writeSamplerDescriptor(Sampler* sampler, void* destination)
        {
            auto dxSampler = static_cast<DX12Sampler*>(sampler);
            D3D12_CPU_DESCRIPTOR_HANDLE handle((size_t)destination);

            _device->CreateSampler(&dxSampler->dx12Desc(),handle);
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
            return new DX12Sampler(this, min, mag, mip, u, v, w, mipLODBias, anisotrophyEnabled, maxAnisotrophy, comparisonFunction, borderColor, minLOD, maxLOD);
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

        Microsoft::WRL::ComPtr<IDXGIFactory4>& DX12GraphicsCard::dxgiFactory()
        {
            return _dxgiFactory;
        }

        ID3D12RootSignature* DX12GraphicsCard::rootSignature()
        {
            return _rootSignature;
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
            std::swap(_cpuReadablePool,from._cpuReadablePool);
            std::swap(_rootSignature,from._rootSignature);
        }
    } // dx12
} // slag
