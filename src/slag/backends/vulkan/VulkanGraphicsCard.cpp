#include "VulkanGraphicsCard.h"

#include "VulkanBackend.h"
#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanResourceDescriptorHeap.h"
#include "VulkanSampler.h"
#include "VulkanSamplerDescriptorHeap.h"
#include "VulkanSemaphore.h"
#include "VulkanShaderPipeline.h"
#include "VulkanSubmissionQueue.h"
#include "VulkanSwapChain.h"
#include "VulkanTexture.h"
#include "slag/exceptions/InvalidShaderCodeError.h"
#include "slag/exceptions/NotImplemented.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace vulkan
    {
        VulkanGraphicsCard::VulkanGraphicsCard(VkInstance instance, const vkb::Device& device)
        {
            _device = device.device;
            _physicalDevice = device.physical_device.physical_device;
            auto physicalDeviceProperties = device.physical_device.properties;
            _name = device.physical_device.name;
            auto physicalDeviceMemoryProperties = device.physical_device.memory_properties;

            //guaranteed to have this, everything will fall back to this if they don't have one
            _graphicsQueueFamily = device.get_queue_index(vkb::QueueType::graphics).value();

             auto tqf = device.get_queue_index(vkb::QueueType::transfer);
            if(tqf.has_value())
            {
                _transferQueueFamily = tqf.value();
            }
            else
            {
                _transferQueueFamily = _graphicsQueueFamily;
            }
            auto cqf = device.get_queue_index(vkb::QueueType::compute);
            if(cqf.has_value())
            {
                _computeQueueFamily = cqf.value();
            }
            else
            {
                _computeQueueFamily = _graphicsQueueFamily;
            }

            auto gqueue = device.get_dedicated_queue(vkb::QueueType::graphics);
            if(gqueue.has_value())
            {
                _graphicsQueue = new VulkanSubmissionQueue(this,gqueue.value(), QueueType::GRAPHICS);
            }
            else
            {
                gqueue = device.get_queue(vkb::QueueType::graphics);
                _graphicsQueue = new VulkanSubmissionQueue(this,gqueue.value(),QueueType::GRAPHICS);
            }

            auto cqueue = device.get_dedicated_queue(vkb::QueueType::compute);
            if(cqueue.has_value())
            {
                _computeQueue = new VulkanSubmissionQueue(this,cqueue.value(), QueueType::COMPUTE);
            }
            else
            {
                cqueue = device.get_queue(vkb::QueueType::compute);
                if(cqueue.has_value())
                {
                    _computeQueue = new VulkanSubmissionQueue(this,cqueue.value(), QueueType::COMPUTE);
                }
                else
                {
                    _computeQueue = _graphicsQueue;
                }
            }

            auto tqueue = device.get_dedicated_queue(vkb::QueueType::transfer);
            if(tqueue.has_value())
            {
                _transferQueue = new VulkanSubmissionQueue(this,tqueue.value(), QueueType::TRANSFER);
            }
            else
            {
                tqueue = device.get_queue(vkb::QueueType::transfer);
                if(tqueue.has_value())
                {
                    _transferQueue = new VulkanSubmissionQueue(this,tqueue.value(), QueueType::TRANSFER);
                }
                else
                {
                    _transferQueue = _computeQueue;
                }
            }


            VmaAllocatorCreateInfo allocatorInfo = {};
            allocatorInfo.physicalDevice = _physicalDevice;
            allocatorInfo.device = _device;
            allocatorInfo.instance = instance;
            allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
            vmaCreateAllocator(&allocatorInfo, &_allocator);

            //Establish memory properties

            for (int i=0; i< physicalDeviceMemoryProperties.memoryHeapCount; i++)
            {
                auto heap = physicalDeviceMemoryProperties.memoryHeaps[i];
                if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
                {
                    _memoryProperties.videoMemory += heap.size;
                }
            }
            _memoryProperties.cacheCoherentSharedMemory = true;
            for (int i=0; i<physicalDeviceMemoryProperties.memoryTypeCount; i++)
            {
                auto type = physicalDeviceMemoryProperties.memoryTypes[i];
                if (type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
                {
                    if ((!(type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) && (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)))
                    {
                        _memoryProperties.cacheCoherentSharedMemory = false;
                        break;
                    }
                }
            }

            _memoryProperties.maxUniformBufferSize = physicalDeviceProperties.limits.maxUniformBufferRange;


            VkPhysicalDeviceProperties2 prop2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
            VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProps{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
            prop2.pNext = &rtProps;
            VkPhysicalDeviceDescriptorHeapPropertiesEXT dhProps{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_PROPERTIES_EXT};
            rtProps.pNext = &dhProps;
            vkGetPhysicalDeviceProperties2(_physicalDevice, &prop2);

            //Establish capabilities
            _capabilities.defragmentable = true;
            _capabilities.raytracing =  rtProps.shaderGroupHandleSize > 0;

            //establish descriptor heap details
            _descriptorHeapDetails.resourceDescriptorIncrementSize = dhProps.imageDescriptorSize;
            _descriptorHeapDetails.samplerDescriptorIncrementSize = dhProps.samplerDescriptorSize;
            _descriptorHeapDetails.maxResourceDescriptors=(dhProps.maxResourceHeapSize-dhProps.minResourceHeapReservedRange) / dhProps.imageDescriptorSize;
            _descriptorHeapDetails.maxSamplerDescriptors=(dhProps.maxSamplerHeapSize-dhProps.minSamplerHeapReservedRange) / dhProps.samplerDescriptorSize;
            _descriptorHeapDetails.resourceReservedRangeSize=dhProps.minResourceHeapReservedRange;
            _descriptorHeapDetails.samplerReservedRangeSize=dhProps.minSamplerHeapReservedRange;


            //load extension methods
            vkCmdBindSamplerHeap=reinterpret_cast<PFN_vkCmdBindSamplerHeapEXT>(vkGetDeviceProcAddr(_device,"vkCmdBindSamplerHeapEXT"));
            vkCmdBindResourceHeap=reinterpret_cast<PFN_vkCmdBindResourceHeapEXT>(vkGetDeviceProcAddr(_device,"vkCmdBindResourceHeapEXT"));
            vkWriteSamplerDescriptors=reinterpret_cast<PFN_vkWriteSamplerDescriptorsEXT>(vkGetDeviceProcAddr(_device,"vkWriteSamplerDescriptorsEXT"));
            vkWriteResourceDescriptors=reinterpret_cast<PFN_vkWriteResourceDescriptorsEXT>(vkGetDeviceProcAddr(_device,"vkWriteResourceDescriptorsEXT"));
            vkRegisterCustomBorderColor=reinterpret_cast<PFN_vkRegisterCustomBorderColorEXT>(vkGetDeviceProcAddr(_device,"vkRegisterCustomBorderColorEXT"));
            vkUnregisterCustomBorderColor=reinterpret_cast<PFN_vkUnregisterCustomBorderColorEXT>(vkGetDeviceProcAddr(_device,"vkUnregisterCustomBorderColorEXT"));
            vkCmdPushData=reinterpret_cast<PFN_vkCmdPushDataEXT>(vkGetDeviceProcAddr(_device,"vkCmdPushDataEXT"));

        }

        VulkanGraphicsCard::~VulkanGraphicsCard()
        {
            if (_device)
            {
                vmaDestroyAllocator(_allocator);
                vkDeviceWaitIdle(_device);
                if (!(_transferQueue == _computeQueue || _transferQueue == _graphicsQueue))
                {
                    delete _transferQueue;
                }
                if (_computeQueue != _graphicsQueue)
                {
                    delete _computeQueue;
                }
                delete _graphicsQueue;
                vkDestroyDevice(_device, nullptr);
            }
        }

        VulkanGraphicsCard::VulkanGraphicsCard(VulkanGraphicsCard&& from) noexcept
        {
            move(from);
        }

        VulkanGraphicsCard& VulkanGraphicsCard::operator=(VulkanGraphicsCard&& from)noexcept
        {
            move(from);
            return *this;
        }

        std::string VulkanGraphicsCard::name() const
        {
            return _name;
        }

        const GraphicsCardMemoryProperties& VulkanGraphicsCard::memoryProperties() const
        {
            return _memoryProperties;
        }

        const GraphicsCardCapabilities& VulkanGraphicsCard::capabilities() const
        {
            return _capabilities;
        }

        const DescriptorHeapDetails& VulkanGraphicsCard::descriptorHeapDetails() const
        {
            return _descriptorHeapDetails;
        }

        PixelFormatProperties VulkanGraphicsCard::formatProperties(PixelFormat format) const
        {
            auto nativeFormat = VulkanBackend::nativeFormat(format);
            PixelFormatProperties properties{};

            VkFormatProperties2 formatProperties{.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2};
            vkGetPhysicalDeviceFormatProperties2(_physicalDevice,nativeFormat.format,&formatProperties);

            VkFormatFeatureFlags2 features = 0;
            TextureUsageFlags usage =static_cast<TextureUsageFlags>(0);

            if (formatProperties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_2_TRANSFER_SRC_BIT && formatProperties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_2_TRANSFER_SRC_BIT)
            {
                properties.tiling = TextureTiling::OPTIMIZED;
                features = formatProperties.formatProperties.optimalTilingFeatures;
            }
            else if (formatProperties.formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_2_TRANSFER_SRC_BIT && formatProperties.formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_2_TRANSFER_SRC_BIT)
            {
                properties.tiling = TextureTiling::LINEAR;
                features = formatProperties.formatProperties.optimalTilingFeatures;
            }
            else
            {
                properties.tiling = TextureTiling::UNSUPPORTED;
                return properties;
            }
            bool hasUsage = false;
            if (features & VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_BIT)
            {
                usage |= TextureUsageFlags::SAMPLED;
                hasUsage = true;
            }
            if (features & VK_FORMAT_FEATURE_2_STORAGE_IMAGE_BIT )
            {
                usage |= TextureUsageFlags::UNORDERED_ACCESS;
                hasUsage = true;
            }
            if (features & VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BIT)
            {
                usage |= TextureUsageFlags::COLOR_TARGET;
                hasUsage = true;
            }
            if (features & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                usage |= TextureUsageFlags::DEPTH_STENCIL_TARGET;
                hasUsage = true;
            }
            properties.validUsageFlags = usage;
            if (!hasUsage)
            {
                properties.tiling = TextureTiling::UNSUPPORTED;
                return properties;
            }
            if (features & VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_FILTER_LINEAR_BIT)
            {
                properties.linearFilteringCapable = true;
            }
            if (features & VK_FORMAT_FEATURE_2_BLIT_SRC_BIT )
            {
                properties.blitSource = true;
            }
            if (features & VK_FORMAT_FEATURE_2_BLIT_DST_BIT )
            {
                properties.blitDestination = true;
            }

            return properties;
        }

        SubmissionQueue* VulkanGraphicsCard::graphicsQueue()
        {
            return _graphicsQueue;
        }

        SubmissionQueue* VulkanGraphicsCard::computeQueue()
        {
            return _computeQueue;
        }

        SubmissionQueue* VulkanGraphicsCard::transferQueue()
        {
            return _transferQueue;
        }

        uint64_t VulkanGraphicsCard::defragmentMemory(
            uint64_t targetBytes,
            std::function<void(MemoryReference*)> memoryMoved)
        {


            VmaDefragmentationInfo defragInfo = {};
            defragInfo.flags = VMA_DEFRAGMENTATION_FLAG_ALGORITHM_BALANCED_BIT;
            defragInfo.maxBytesPerPass = targetBytes;


            VmaDefragmentationContext defragCtx;
            VkResult res = vmaBeginDefragmentation(_allocator, &defragInfo, &defragCtx);
            if (res != VK_SUCCESS)
            {
                throw std::runtime_error("failed to begin defragmentation");
            }
            while (true)
            {
                VmaDefragmentationPassMoveInfo pass;
                res = vmaBeginDefragmentationPass(_allocator, defragCtx, &pass);
                if(res == VK_SUCCESS)
                    break;
                else if(res == VK_INCOMPLETE)
                {
                    std::vector<VulkanBufferMoveData> movedBuffers;
                    std::vector<VulkanImageMoveData> movedTextures;
                    std::vector<MemoryReference*> movedMemoryRefs;
                    VulkanCommandBuffer transitionCB(this,QueueType::GRAPHICS);
                    VulkanCommandBuffer moveCB(this,QueueType::GRAPHICS);
                    VulkanSemaphore transitioned(this,0);
                    VulkanSemaphore moved(this,0);
                    transitionCB.begin();
                    moveCB.begin();
                    for(uint32_t i = 0; i < pass.moveCount; ++i)
                    {
                        // Inspect pass.pMoves[i].srcAllocation, identify what buffer/image it represents.
                        VmaAllocationInfo allocInfo;
                        vmaGetAllocationInfo(_allocator, pass.pMoves[i].srcAllocation, &allocInfo);
                        MemoryReference* userData = (MemoryReference*)allocInfo.pUserData;
                        if (userData->type == MemoryObjectType::TEXTURE)
                        {
                            auto texture = static_cast<VulkanTexture*>(userData->memory.texture);
                            auto movedTexture = texture->moveMemory(pass.pMoves[i].dstTmpAllocation,&transitionCB,&moveCB);
                            if (movedTexture.movedSucceded)
                            {
                                movedTextures.push_back(movedTexture);
                                movedMemoryRefs.push_back(userData);
                            }
                            else
                            {
                                pass.pMoves[i].operation = VMA_DEFRAGMENTATION_MOVE_OPERATION_IGNORE;
                            }
                        }
                        else
                        {
                            auto buffer = static_cast<VulkanBuffer*>(userData->memory.buffer);
                            auto movedBuffer = buffer->moveMemory(pass.pMoves[i].dstTmpAllocation,&moveCB);
                            if (movedBuffer.movedSucceded)
                            {
                                movedBuffers.push_back(movedBuffer);
                                movedMemoryRefs.push_back(userData);
                            }
                            else
                            {
                                pass.pMoves[i].operation = VMA_DEFRAGMENTATION_MOVE_OPERATION_IGNORE;
                            }
                        }

                    }
                    transitionCB.end();
                    moveCB.end();
                    CommandBuffer* transitionedPtr = &transitionCB;
                    CommandBuffer* movedPtr = &moveCB;
                    SemaphoreValue transitionedValue{.semaphore = &transitioned,.value = 1};
                    SemaphoreValue movedValue{.semaphore = &moved,.value = 1};
                    SubmissionBatch batches[]
                    {
                        {
                            .waitSemaphores = nullptr,
                            .waitSemaphoreCount = 0,
                            .commandBuffers = &transitionedPtr,
                            .commandBufferCount = 1,
                            .signalSemaphores = &transitionedValue,
                            .signalSemaphoreCount = 1,
                        },
                        {
                            .waitSemaphores = &transitionedValue,
                            .waitSemaphoreCount = 1,
                            .commandBuffers = &movedPtr,
                            .commandBufferCount = 1,
                            .signalSemaphores = &movedValue,
                            .signalSemaphoreCount = 1,
                        }
                    };

                    _graphicsQueue->submit(batches,2);
                    moved.waitForValue(1);
                    for (auto i=0; i< movedTextures.size(); i++)
                    {
                        vkDestroyImage(_device,movedTextures[i].image,nullptr);
                    }
                    movedTextures.clear();
                    for (auto i=0; i< movedBuffers.size(); i++)
                    {
                        vkDestroyBuffer(_device,movedBuffers[i].buffer,nullptr);
                    }
                    movedBuffers.clear();
                    for (auto i=0; i< movedMemoryRefs.size(); i++)
                    {
                        if (movedMemoryRefs[i]->type == MemoryObjectType::BUFFER)
                        {
                            static_cast<VulkanBuffer*>(movedMemoryRefs[i]->memory.buffer)->updatePointer();
                        }
                        if (memoryMoved != nullptr)
                        {
                            memoryMoved(movedMemoryRefs[i]);
                        }
                    }

                    movedMemoryRefs.clear();
                    res = vmaEndDefragmentationPass(_allocator, defragCtx, &pass);
                    if(res == VK_SUCCESS)
                    {
                        break;
                    }
                    else if(res != VK_INCOMPLETE)
                    {
                        throw std::runtime_error("failed to defragment graphics memory");
                    }

                }
                else
                {
                    throw std::runtime_error("failed to defragment graphics memory");
                }
            }
            VmaDefragmentationStats stats{};
            vmaEndDefragmentation(_allocator,defragCtx,&stats);
            return stats.bytesMoved;
        }

        SwapChain* VulkanGraphicsCard::newSwapchain(const PlatformData& platformData, uint32_t width, uint32_t height, const SwapChainParameters& parameters)
        {
            return new VulkanSwapChain(this,platformData,width,height,parameters);
        }

        ShaderPipeline* VulkanGraphicsCard::newShaderPipeline(
            const VertexDescription& vertexDescription,
            const ShaderCode& vertexShader,
            const ShaderCode& fragmentShader,
            const PipelineState& pipelineState,
            const FramebufferDescription& framebufferDescription)
        {
            return new VulkanShaderPipeline(this,vertexDescription,vertexShader,fragmentShader,pipelineState,framebufferDescription);
        }

        CommandBuffer* VulkanGraphicsCard::newCommandBuffer(QueueType type)
        {
            return new VulkanCommandBuffer(this, type);
        }

        Semaphore* VulkanGraphicsCard::newSemaphore(uint64_t initialValue)
        {
           return new VulkanSemaphore(this,initialValue);
        }

        Buffer* VulkanGraphicsCard::newBuffer(
            uint64_t size,
            BufferCPUAccess cpuAccess,
            BufferMemoryType memoryType)
        {
            return new VulkanBuffer(this, size, cpuAccess, memoryType);
        }

        ResourceDescriptorHeap* VulkanGraphicsCard::newResourceDescriptorHeap(uint32_t descriptorCount)
        {
            return new VulkanResourceDescriptorHeap(this,descriptorCount);
        }

        SamplerDescriptorHeap* VulkanGraphicsCard::newSamplerDescriptorHeap(uint32_t descriptorCount)
        {
            return new VulkanSamplerDescriptorHeap(this,descriptorCount);
        }

        Texture* VulkanGraphicsCard::newTexture1D(uint32_t width, PixelFormat format, TextureUsageFlags usage, uint32_t mipLevels, uint32_t layers)
        {
            return new VulkanTexture(this,width,format,usage,mipLevels,layers);
        }

        Texture* VulkanGraphicsCard::newTexture2D(uint32_t width, uint32_t height, PixelFormat format, TextureUsageFlags usage, uint32_t mipLevels,
            SampleCount sampleCount, uint32_t layers)
        {
            return new VulkanTexture(this,width,height,format,usage,mipLevels,sampleCount,layers);
        }

        Texture* VulkanGraphicsCard::newTexture3D(uint32_t width, uint32_t height, uint32_t depth, PixelFormat format, TextureUsageFlags usage,
            uint32_t mipLevels)
        {
            return new VulkanTexture(this,width,height,depth,format,usage,mipLevels);
        }

        Texture* VulkanGraphicsCard::newTextureCube(uint32_t dimension, PixelFormat format, TextureUsageFlags usage, uint32_t mipLevels,
            uint32_t arrayDepth)
        {
            return new VulkanTexture(this,format,usage,dimension,mipLevels,arrayDepth);
        }

        Sampler* VulkanGraphicsCard::newSampler(
            SamplerFilter min,
            SamplerFilter mag,
            SamplerFilter mip,
            SamplerAddressMode u,
            SamplerAddressMode v,
            SamplerAddressMode w,
            float mipLODBias,
            bool anisotrophyEnabled,
            uint8_t maxAnisotrophy,
            ComparisonFunction comparisonFunction,
            Color borderColor,
            float minLOD,
            float maxLOD)
        {
            return new VulkanSampler(this,min,mag,mip,u,v,w,mipLODBias,anisotrophyEnabled,maxAnisotrophy,comparisonFunction,borderColor,minLOD,maxLOD);
        }

        void VulkanGraphicsCard::move(VulkanGraphicsCard& from)
        {
            _name.swap(from._name);
            _memoryProperties = from._memoryProperties;
            _capabilities = from._capabilities;
            _descriptorHeapDetails = from._descriptorHeapDetails;
            std::swap(_physicalDevice,from._physicalDevice);
            std::swap(_device,from._device);
            std::swap(_allocator,from._allocator);
            std::swap(_graphicsQueue,from._graphicsQueue);
            std::swap(_computeQueue,from._computeQueue);
            std::swap(_transferQueue,from._transferQueue);
            _graphicsQueueFamily=from._graphicsQueueFamily;
            _computeQueueFamily=from._computeQueueFamily;
            _transferQueueFamily=from._transferQueueFamily;
            _allocatedSamplers=from._allocatedSamplers;


            vkCmdBindSamplerHeap=from.vkCmdBindSamplerHeap;
            vkCmdBindResourceHeap=from.vkCmdBindResourceHeap;
            vkWriteSamplerDescriptors=from.vkWriteSamplerDescriptors;
            vkWriteResourceDescriptors=from.vkWriteResourceDescriptors;
            vkRegisterCustomBorderColor=from.vkRegisterCustomBorderColor;
            vkUnregisterCustomBorderColor=from.vkUnregisterCustomBorderColor;
            vkCmdPushData=from.vkCmdPushData;
        }

        VkDevice VulkanGraphicsCard::device() const
        {
            return _device;
        }

        VkPhysicalDevice VulkanGraphicsCard::physicalDevice() const
        {
            return _physicalDevice;
        }

        VmaAllocator VulkanGraphicsCard::allocator() const
        {
            return _allocator;
        }

        uint32_t VulkanGraphicsCard::graphicsFamilyIndex() const
        {
            return _graphicsQueueFamily;
        }

        uint32_t VulkanGraphicsCard::computeFamilyIndex() const
        {
            return _computeQueueFamily;
        }

        uint32_t VulkanGraphicsCard::transferFamilyIndex() const
        {
            return _transferQueueFamily;
        }
    } // vulkan
} // slag
