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
                insertTexture(kvpair.first,kvpair.second);
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
                insertVertexBufferResource(kvpair.first,kvpair.second);
            }
            for(auto kvpair: indexBufferDescriptions)
            {
                insertIndexBufferResource(kvpair.first,kvpair.second);
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

        Texture *VulkanFrame::getTextureResource(std::string resourceName)
        {
            return &_textureResources.at(resourceName);
        }

        CommandBuffer *VulkanFrame::getCommandBufferResource(std::string resourceName)
        {
            return &_commandBufferResources.at(resourceName);
        }

        UniformBuffer *VulkanFrame::getUniformBufferResource(std::string resourceName)
        {
            return &_uniformBufferResources.at(resourceName);
        }

        VertexBuffer *VulkanFrame::getVertexBufferResource(std::string resourceName)
        {
            return _vertexBufferResources.at(resourceName);
        }

        IndexBuffer *VulkanFrame::getIndexBufferResource(std::string resourceName)
        {
            return _indexBufferResources.at(resourceName);
        }

        void VulkanFrame::updateTextureResource(const std::string resourceName, const TextureResourceDescription &description)
        {
            if(_textureResources.contains(resourceName))
            {
                _textureResources.erase(resourceName);
            }
            insertTexture(resourceName,description);
        }

        void VulkanFrame::updateVertexBufferResource(const std::string resourceName, const VertexBufferResourceDescription &description)
        {
            if(_vertexBufferResources.contains(resourceName))
            {
                _vertexBufferResources.erase(resourceName);
            }
            insertVertexBufferResource(resourceName,description);
        }

        void VulkanFrame::updateIndexBufferResource(const std::string resourceName, const IndexBufferResourceDescription &description)
        {
            if(_indexBufferResources.contains(resourceName))
            {
                _indexBufferResources.erase(resourceName);
            }
            insertIndexBufferResource(resourceName,description);
        }

        void VulkanFrame::insertTexture(std::string name, TextureResourceDescription description)
        {
            uint32_t w=0;
            uint32_t h=0;
            if(description.sizingMode == TextureResourceDescription::FrameRelative)
            {
                w = _fromSwapChain->width() * description.width;
                h = _fromSwapChain->height() * description.height;
            }
            else
            {
                w = description.width;
                h = description.height;
            }
            _textureResources.insert(std::make_pair(name, VulkanTexture(w,h,1,VulkanTexture::usageFromCrossPlatform(description.usage),description.format,Texture::Layout::SHADER_RESOURCE,description.features,true)));
        }

        void VulkanFrame::insertVertexBufferResource(std::string name, VertexBufferResourceDescription description)
        {
            VulkanVertexBuffer* vb = nullptr;
            std::vector<unsigned char> empty(description.defaultSize,0);
            if(description.usage == Buffer::CPU)
            {
                vb = new VulkanCPUVertexBuffer(empty.data(),empty.size(), true);
            }
            else
            {
                vb = new VulkanGPUVertexBuffer(empty.data(),empty.size(), true);
            }
            _vertexBufferResources.insert(std::make_pair(name, vb));
        }

        void VulkanFrame::insertIndexBufferResource(std::string name, IndexBufferResourceDescription description)
        {
            VulkanIndexBuffer* ib = nullptr;
            std::vector<unsigned char> empty(description.defaultSize,0);
            if(description.usage == Buffer::CPU)
            {
                ib = new VulkanCPUIndexBuffer(empty.data(),empty.size(), true);
            }
            else
            {
                ib = new VulkanGPUIndexBuffer(empty.data(),empty.size(), true);
            }
            _indexBufferResources.insert(std::make_pair(name, ib));
        }




    } // slag
} // vulkan