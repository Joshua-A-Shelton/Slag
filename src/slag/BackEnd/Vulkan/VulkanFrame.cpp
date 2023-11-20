#include <iostream>
#include "VulkanSwapchain.h"
#include "VulkanFrame.h"
#include "VulkanLib.h"
#include "VulkanCPUVertexBuffer.h"
#include "VulkanGPUVertexBuffer.h"
#include "VulkanCPUIndexBuffer.h"
#include "VulkanGPUIndexBuffer.h"


namespace slag
{
    namespace vulkan
    {
        VulkanFrame::VulkanFrame(VulkanSwapchain* from,
                                 VkDeviceSize uniformBufferStartSize,
                                 const std::unordered_map<std::string,TextureResourceDescription>& textureDescriptions,
                                 const std::unordered_set<std::string>& commandBufferNames,
                                 const std::unordered_map<std::string,UniformBufferResourceDescription>& uniformBufferDescriptions,
                                 const std::unordered_map<std::string, VertexBufferResourceDescription>& vertexBufferDescriptions,
                                 const std::unordered_map<std::string, IndexBufferResourceDescription>& indexBufferDescriptions)
                                 : _commandBuffer(nullptr, true, nullptr, true), _virtualUniformBuffer(uniformBufferStartSize, true)
        {
            assert(from != nullptr && "From swapchain cannot be null!");
            _fromSwapChain = from;
            auto commandPool = _fromSwapChain->commandPool();

            _commandBuffer = std::move(VulkanCommandBuffer(commandPool, true, VulkanLib::graphicsCard()->graphicsQueue(), true));

            //in flight fence, render and image available semaphores
            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            auto result = vkCreateSemaphore(VulkanLib::graphicsCard()->device(),&semaphoreInfo, nullptr,&_imageAvailable);
            assert(result == VK_SUCCESS && "failed to create semaphore!");
            result = vkCreateSemaphore(VulkanLib::graphicsCard()->device(),&semaphoreInfo, nullptr,&_renderFinished);
            assert(result == VK_SUCCESS && "failed to create semaphore!");
            result = vkCreateFence(VulkanLib::graphicsCard()->device(), &fenceInfo, nullptr,&_inFlight);
            assert(result == VK_SUCCESS && "failed to create fence!");

            for(auto& kvpair : textureDescriptions)
            {
                uint32_t w=0;
                uint32_t h=0;
                if(kvpair.second.sizingMode == TextureResourceDescription::FRAME_RELATIVE)
                {
                    w = from->width() * kvpair.second.width;
                    h = from->height() * kvpair.second.height;
                }
                else
                {
                    w = kvpair.second.width;
                    h = kvpair.second.height;
                }
                _textureResources.insert(std::make_pair(kvpair.first, VulkanTexture(w,h,1,VulkanTexture::usageFromCrossPlatform(kvpair.second.usage),kvpair.second.format, true)));
            }
            for(auto& name : commandBufferNames)
            {
                _commandBufferResources.insert(std::make_pair(name,VulkanCommandBuffer(false,VulkanLib::graphicsCard()->graphicsQueue(),VulkanLib::graphicsCard()->graphicsQueueFamily(), true)));
            }
            for(auto& kvpair : uniformBufferDescriptions)
            {
                _uniformBufferResources.insert(std::make_pair(kvpair.first, VulkanVirtualUniformBuffer(kvpair.second.defaultSize, true)));
            }
            for(auto& kvpair : vertexBufferDescriptions)
            {
                VulkanVertexBuffer* vb = nullptr;
                std::vector<unsigned char> empty(kvpair.second.defaultSize,0);
                if(kvpair.second.usage == Buffer::CPU)
                {
                    vb = new VulkanCPUVertexBuffer(empty.data(),empty.size());
                }
                else
                {
                    vb = new VulkanGPUVertexBuffer(empty.data(),empty.size());
                }
                _vertexBufferResources.insert(std::make_pair(kvpair.first, vb));
            }
            for(auto kvpair: indexBufferDescriptions)
            {
                VulkanIndexBuffer* ib = nullptr;
                std::vector<unsigned char> empty(kvpair.second.defaultSize,0);
                if(kvpair.second.usage == Buffer::CPU)
                {
                    ib = new VulkanCPUIndexBuffer(empty.data(),empty.size());
                }
                else
                {
                    ib = new VulkanGPUIndexBuffer(empty.data(),empty.size());
                }
                _indexBufferResources.insert(std::make_pair(kvpair.first, ib));
            }
        }

        VulkanFrame::~VulkanFrame()
        {
            //these are pointers to objects in memory, not the objects themselves, so we have to delete here
            for(auto& buffer : _vertexBufferResources)
            {
                delete buffer.second;
            }
            //these are pointers to objects in memory, not the objects themselves, so we have to delete here
            for(auto& buffer : _indexBufferResources)
            {
                delete buffer.second;
            }
            if(_inFlight)
            {
                vkDestroyFence(VulkanLib::graphicsCard()->device(),_inFlight, nullptr);
                vkDestroySemaphore(VulkanLib::graphicsCard()->device(),_imageAvailable, nullptr);
                vkDestroySemaphore(VulkanLib::graphicsCard()->device(),_renderFinished, nullptr);
                freeResourceReferences();
            }

        }

        VulkanFrame::VulkanFrame(VulkanFrame&& from): _commandBuffer(nullptr, true, nullptr, true), _virtualUniformBuffer(0, true)
        {
            move(std::move(from));
        }
        VulkanFrame& VulkanFrame::operator=(VulkanFrame&& from)
        {
            move(std::move(from));
            return *this;
        }

        void VulkanFrame::move(VulkanFrame&& from)
        {
            std::swap(_swapchainImageTexture , from._swapchainImageTexture);
            std::swap(_inFlight , from._inFlight);
            std::swap(_imageAvailable, from._imageAvailable);
            std::swap(_renderFinished, from._renderFinished);
            std::swap(_fromSwapChain, from._fromSwapChain);
            std::swap(_descriptorAllocator,from._descriptorAllocator);
            _virtualUniformBuffer = std::move(from._virtualUniformBuffer);
            _commandBuffer = std::move(from._commandBuffer);
            _textureResources.swap(from._textureResources);
            _commandBufferResources.swap(from._commandBufferResources);
            _uniformBufferResources.swap(from._uniformBufferResources);
            _vertexBufferResources.swap(from._vertexBufferResources);
            _indexBufferResources.swap(from._indexBufferResources);
        }

        void VulkanFrame::waitTillFinished()
        {
            auto result = vkWaitForFences(VulkanLib::graphicsCard()->device(),1,&_inFlight, true, 1000000000);
            assert(result == VK_SUCCESS && "render fence wait timed out");
        }

        void VulkanFrame::resetWait()
        {
            auto result = vkResetFences(VulkanLib::graphicsCard()->device(),1,&_inFlight);
            assert(result == VK_SUCCESS && "render fence could not be reset");
            //kinda a weird spot for it, but it's technically right....
            freeResourceReferences();
        }

        VkSemaphore VulkanFrame::renderFinishedSemaphore()
        {
            return _renderFinished;
        }

        VkSemaphore VulkanFrame::imageAvailableSemaphore()
        {
            return _imageAvailable;
        }

        CommandBuffer* VulkanFrame::getCommandBuffer()
        {
            return &_commandBuffer;
        }

        UniformBuffer *VulkanFrame::getUniformBuffer()
        {
            return &_virtualUniformBuffer;
        }

        Texture *VulkanFrame::getBackBuffer()
        {
            return _swapchainImageTexture;
        }

        void VulkanFrame::begin()
        {
            _descriptorAllocator.resetPools();
            _commandBuffer.reset();
            _virtualUniformBuffer.reset();
            _commandBuffer.begin();
        }

        void VulkanFrame::end()
        {
            _commandBuffer.end();

            _commandBuffer.submit(&_imageAvailable,1,&_renderFinished,1,_inFlight);
            _fromSwapChain->queueToPresent(this);
        }

        void VulkanFrame::setSwapchainImageTexture(VulkanTexture* texture)
        {
            _swapchainImageTexture = texture;
        }

        VkDeviceSize VulkanFrame::uniformBufferSize()
        {
            return _virtualUniformBuffer.virtualSize();
        }

        UniformSetDataAllocator *VulkanFrame::getUniformSetDataAllocator()
        {
            return &_descriptorAllocator;
        }


    } // slag
} // vulkan