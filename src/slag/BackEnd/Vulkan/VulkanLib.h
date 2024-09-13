#ifndef SLAG_VULKANLIB_H
#define SLAG_VULKANLIB_H
#include "../BackEndLib.h"
#include "VulkanGraphicsCard.h"
#include <vulkan/vulkan.h>

namespace slag
{
    namespace vulkan
    {

        struct VulkanizedFormat
        {
            VkFormat format = VK_FORMAT_UNDEFINED;
            VkComponentMapping mapping{};
        };
        class VulkanLib: public lib::BackEndLib
        {
        public:
            static VulkanLib* initialize();
            static VulkanLib* get();
            static void cleanup(lib::BackEndLib* library);
            static VulkanGraphicsCard* card();

            static VulkanizedFormat format(Pixels::Format format);
            static VkImageLayout layout(Texture::Layout layout);
            static VkImageUsageFlags imageUsage(Texture::Usage usage);

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
            //Swapchain
            Swapchain* newSwapchain(PlatformData platformData, uint32_t width, uint32_t height, uint8_t backBuffers, Swapchain::PresentMode mode, Pixels::Format imageFormat)override;
            //Textures
            Texture* newTexture(void* texelData, size_t dataSize, Pixels::Format dataFormat, Pixels::Format textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, Texture::Usage usage, Texture::Layout initializedLayout, bool generateMips)override;
            Texture* newTexture(CommandBuffer* onBuffer, void* texelData, size_t dataSize, Pixels::Format dataFormat, Pixels::Format textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, Texture::Usage usage, Texture::Layout initializedLayout, bool generateMips)override;
            //CommandBuffers
            CommandBuffer* newCommandBuffer(GpuQueue::QueueType acceptsCommands);
            //Buffers
            Buffer* newBuffer(void* data, size_t dataSize, Buffer::Accessibility accessibility, Buffer::Usage usage)override;
            Buffer* newBuffer(size_t  bufferSize, Buffer::Accessibility accessibility, Buffer::Usage usage)override;
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
