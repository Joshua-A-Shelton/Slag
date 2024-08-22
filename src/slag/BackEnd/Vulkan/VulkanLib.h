#ifndef SLAG_VULKANLIB_H
#define SLAG_VULKANLIB_H
#include "../BackEndLib.h"
#include "VulkanGraphicsCard.h"
#include <vulkan/vulkan.h>

namespace slag
{
    namespace vulkan
    {

        class VulkanLib: public lib::BackEndLib
        {
        public:
            static VulkanLib* initialize();
            static VulkanLib* get();
            static void cleanup(lib::BackEndLib* library);
            static VulkanGraphicsCard* card();

            VulkanLib(VkInstance instance, VkDebugUtilsMessengerEXT messenger, VulkanGraphicsCard* card);
            ~VulkanLib();
            VulkanLib(const VulkanLib&)=delete;
            VulkanLib& operator=(const VulkanLib&)=delete;
            VulkanLib(VulkanLib&&)=delete;
            VulkanLib& operator=(VulkanLib&&)=delete;

            VkInstance instance();
            BackEnd identifier()override;
            GraphicsCard* graphicsCard()override;
            VulkanGraphicsCard* vulkanGraphicsCard();
            //Textures
            Texture* newTexture(GpuQueue* queue,void* data, Pixels::Format dataFormat, Pixels::Format textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, Texture::Usage usage, Texture::Layout initializedLayout)override;
            //CommandBuffers
            CommandBuffer* newCommandBuffer(GpuQueue::QueueType acceptsCommands);
            //Semaphores
            Semaphore* newSemaphore(uint64_t startingValue)override;
            void waitFor(SemaphoreValue* values, size_t count)override;

        private:
            VkInstance _instance;
            VkDebugUtilsMessengerEXT  _debugMessenger = nullptr;
            VulkanGraphicsCard* _graphicsCard;
        };

    } // vulkan
} // slag

#endif //SLAG_VULKANLIB_H
