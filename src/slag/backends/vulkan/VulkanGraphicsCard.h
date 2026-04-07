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
            [[nodiscard]] const GraphicsCardMemoryProperties& memoryProperties()const override;
            [[nodiscard]] const GraphicsCardCapabilities& capabilities()const override;
            [[nodiscard]] const DescriptorTableDetails& descriptorTableDetails()const override;
            [[nodiscard]] PixelFormatProperties formatProperties(PixelFormat format)const override;
            [[nodiscard]] SubmissionQueue* graphicsQueue()override;
            [[nodiscard]] SubmissionQueue* computeQueue()override;
            [[nodiscard]] SubmissionQueue* transferQueue()override;
            uint64_t defragmentMemory(uint64_t targetBytes, std::function<void(MemoryReference*)> memoryMoved)override;
            //Shaders
            [[nodiscard]] ShaderModule* newShaderModule(ShaderLanguage language, void* data, uint32_t dataLength)override;
            [[nodiscard]] ShaderPipeline* newShaderPipeline(
                const VertexDescription& vertexDescription,
                ShaderModule* vertexShader,
                ShaderModule* fragmentShader,
                const PipelineState& pipelineState,
                const FramebufferDescription& framebufferDescription)override;

            //Command Buffers
            [[nodiscard]] CommandBuffer* newCommandBuffer(QueueType type)override;
            //Semaphores
            [[nodiscard]] Semaphore* newSemaphore(uint64_t initialValue)override;
            //Buffers
            [[nodiscard]] Buffer* newBuffer(
                uint64_t size,
                BufferCPUAccess cpuAccess,
                BufferMemoryType memoryType)override;
            //Textures
            [[nodiscard]] virtual Texture* newTexture1D(
            uint32_t width,
            PixelFormat format,
            TextureUsageFlags usage,
            uint32_t mipLevels,
            uint32_t layers)override;

            [[nodiscard]] virtual Texture* newTexture2D(
                uint32_t width,
                uint32_t height,
                PixelFormat format,
                TextureUsageFlags usage,
                uint32_t mipLevels,
                SampleCount sampleCount,
                uint32_t layers)override;

            [[nodiscard]] virtual Texture* newTexture3D(
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
            GraphicsCardMemoryProperties _memoryProperties{};
            GraphicsCardCapabilities _capabilities{};
            DescriptorTableDetails _descriptorTableDetails{};
            VkPhysicalDevice _physicalDevice=nullptr;
            VkDevice _device=nullptr;
            VmaAllocator _allocator = nullptr;
            VulkanSubmissionQueue* _graphicsQueue=nullptr;
            VulkanSubmissionQueue* _computeQueue=nullptr;
            VulkanSubmissionQueue* _transferQueue=nullptr;
            uint32_t _graphicsQueueFamily=0;
            uint32_t _computeQueueFamily=0;
            uint32_t _transferQueueFamily=0;


        };
    } // vulkan
} // slag

#endif //SLAG_VULKANGRAPHICSCARD_H
