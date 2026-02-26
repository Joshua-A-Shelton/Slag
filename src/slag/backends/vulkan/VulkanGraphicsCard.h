#ifndef SLAG_VULKANGRAPHICSCARD_H
#define SLAG_VULKANGRAPHICSCARD_H
#include <slag/Slag.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <VkBootstrap.h>

namespace slag
{
    namespace vulkan
    {
        class VulkanSubmissionQueue;

        class VulkanGraphicsCard: public GraphicsCard
        {
        public:
            VulkanGraphicsCard(VkInstance instance, const vkb::Device& device);
            ~VulkanGraphicsCard()override;
            VulkanGraphicsCard(const VulkanGraphicsCard&) = delete;
            VulkanGraphicsCard& operator=(const VulkanGraphicsCard&) = delete;
            VulkanGraphicsCard(VulkanGraphicsCard&& from);
            VulkanGraphicsCard& operator=(VulkanGraphicsCard&& from);
            [[nodiscard]] std::string name()const override;
            [[nodiscard]] uint64_t videoMemory()const override;
            [[nodiscard]] uint64_t maxShaderAccessUniformBufferSize()const override;
            [[nodiscard]] PixelFormatProperties formatProperties(PixelFormat format)const override;
            [[nodiscard]] bool cacheCoherentSharedMemory()const override;
            [[nodiscard]] SubmissionQueue* graphicsQueue()override;
            [[nodiscard]] SubmissionQueue* computeQueue()override;
            [[nodiscard]] SubmissionQueue* transferQueue()override;
            uint64_t defragmentMemory(SemaphoreValue* waitFor, uint32_t waitCount, SemaphoreValue* signal, uint32_t signalCount, uint64_t targetBytes=0)override;
            //Command Buffers
            [[nodiscard]] CommandBuffer* newCommandBuffer(QueueType type)override;
            //Semaphores
            [[nodiscard]] Semaphore* newSemaphore(uint64_t initialValue=0)override;
            //Buffers
            [[nodiscard]] Buffer* newBuffer(
                uint64_t size,
                BufferMemoryType memoryType = BufferMemoryType::GENERAL,
                BufferCPUAccess cpuAccess = BufferCPUAccess::WRITE_ONLY)override;
            //Textures
            [[nodiscard]] virtual Texture* newTexture(
            uint32_t width,
            PixelFormat format,
            TextureUsageFlags usage,
            uint32_t mipLevels,
            uint32_t layers)override;

            [[nodiscard]] virtual Texture* newTexture(
                uint32_t width,
                uint32_t height,
                PixelFormat format,
                TextureUsageFlags usage,
                uint32_t mipLevels,
                SampleCount sampleCount,
                uint32_t layers)override;

            [[nodiscard]] virtual Texture* newTexture(
                uint32_t width,
                uint32_t height,
                uint32_t depth,
                PixelFormat format,
                TextureUsageFlags usage,
                uint32_t mipLevels)override;

            [[nodiscard]] virtual Texture* newTextureCube(
                uint32_t dimension,
                PixelFormat format,
                TextureUsageFlags usage,
                uint32_t mipLevels,
                uint32_t arrayDepth
                )override;

            //Vulkan specific
            VkDevice device() const;
            VmaAllocator allocator() const;
            uint32_t graphicsFamilyIndex()const;
            uint32_t computeFamilyIndex()const;
            uint32_t transferFamilyIndex()const;
        private:
            void move(VulkanGraphicsCard& from);
            std::string _name;
            VkPhysicalDevice _physicalDevice=nullptr;
            VkDevice _device=nullptr;
            VkPhysicalDeviceProperties _physicalDeviceProperties={};
            VkPhysicalDeviceMemoryProperties _physicalDeviceMemoryProperties={};
            VmaAllocator _allocator = nullptr;
            VulkanSubmissionQueue* _graphicsQueue=nullptr;
            VulkanSubmissionQueue* _computeQueue=nullptr;
            VulkanSubmissionQueue* _transferQueue=nullptr;
            uint64_t _videoMemory = 0;
            uint32_t _graphicsQueueFamily=0;
            uint32_t _computeQueueFamily=0;
            uint32_t _transferQueueFamily=0;
            bool _cacheCoherentSharedMemory = false;

        };
    } // vulkan
} // slag

#endif //SLAG_VULKANGRAPHICSCARD_H
