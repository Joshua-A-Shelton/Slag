#ifndef SLAG_VULKANFRAME_H
#define SLAG_VULKANFRAME_H
#include "../../Frame.h"
#include "VulkanTexture.h"
#include "VulkanCommandBuffer.h"
#include "VulkanDescriptorAllocator.h"
#include "VulkanVirtualUniformBuffer.h"
#include "VulkanIndexBuffer.h"
#include "VulkanVertexBuffer.h"
#include <vulkan/vulkan.h>

namespace slag
{
    namespace vulkan
    {
        class VulkanSwapchain;
        class VulkanFrame: public Frame
        {
        public:
            VulkanFrame(
                    VulkanSwapchain* from,
                    VkDeviceSize uniformBufferStartSize,
                    const std::unordered_map<std::string,TextureResourceDescription>& textureDescriptions,
                    const std::unordered_set<std::string>& commandBufferNames,
                    const std::unordered_map<std::string,UniformBufferResourceDescription>& uniformBufferDescriptions,
                    const std::unordered_map<std::string, VertexBufferResourceDescription>& vertexBufferDescriptions,
                    const std::unordered_map<std::string, IndexBufferResourceDescription>& indexBufferDescriptions);
            VulkanFrame(const VulkanFrame&)=delete;
            VulkanFrame(VulkanFrame&& from);
            VulkanFrame& operator=(VulkanFrame&& from);
            VulkanFrame& operator=(const VulkanFrame&)=delete;
            ~VulkanFrame() override;
            void begin() override;
            void end() override;
            CommandBuffer* getCommandBuffer() override;
            Texture* getBackBuffer()override;
            UniformBuffer* getUniformBuffer()override;
            UniformSetDataAllocator* getUniformSetDataAllocator()override;
            Texture* getTextureResource(std::string resourceName)override;
            CommandBuffer* getCommandBufferResource(std::string resourceName)override;
            UniformBuffer* getUniformBufferResource(std::string resourceName)override;
            VertexBuffer* getVertexBufferResource(std::string resourceName)override;
            IndexBuffer* getIndexBufferResource(std::string resourceName)override;

            VkSemaphore renderFinishedSemaphore();
            VkSemaphore imageAvailableSemaphore();
            ///only call from swapchain itself
            void setSwapchainImageTexture(VulkanTexture* texture);

            VkDeviceSize uniformBufferSize();

            void waitTillFinished();
            void resetWait();

            void updateTextureResource(const std::string resourceName,const TextureResourceDescription& description);
            void updateVertexBufferResource(const std::string resourceName, const VertexBufferResourceDescription& description);
            void updateIndexBufferResource(const std::string resourceName, const IndexBufferResourceDescription& description);

        private:
            //is the command buffer done (null doesn't mean not done, it means the synchronization object doesn't exist)
            VkFence _inFlight = nullptr;
            VkSemaphore _renderFinished = nullptr;
            VkSemaphore _imageAvailable = nullptr;
            VulkanSwapchain* _fromSwapChain = nullptr;
            VulkanVirtualUniformBuffer _virtualUniformBuffer;
            VulkanTexture* _swapchainImageTexture;
            VulkanCommandBuffer _commandBuffer;
            VulkanDescriptorAllocator _descriptorAllocator;
            std::unordered_map<std::string,VulkanTexture> _textureResources;
            std::unordered_map<std::string,VulkanCommandBuffer> _commandBufferResources;
            std::unordered_map<std::string,VulkanVirtualUniformBuffer> _uniformBufferResources;
            std::unordered_map<std::string,VulkanVertexBuffer*> _vertexBufferResources;
            std::unordered_map<std::string,VulkanIndexBuffer*> _indexBufferResources;
            void move(VulkanFrame&& from);

            void insertTexture(std::string name, TextureResourceDescription description);
            void insertVertexBufferResource(std::string name, VertexBufferResourceDescription description);
            void insertIndexBufferResource(std::string name, IndexBufferResourceDescription description);

        };
    } // slag
} // vulkan
#endif //SLAG_VULKANFRAME_H