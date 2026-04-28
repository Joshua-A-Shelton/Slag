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
            VulkanGraphicsCard(VulkanGraphicsCard&& from) noexcept ;
            VulkanGraphicsCard& operator=(VulkanGraphicsCard&& from) noexcept ;
            [[nodiscard]] std::string name()const override;
            [[nodiscard]] const GraphicsCardMemoryProperties& memoryProperties()const override;
            [[nodiscard]] const GraphicsCardCapabilities& capabilities()const override;
            [[nodiscard]] const DescriptorHeapDetails& descriptorHeapDetails()const override;
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
                PipelineInputMapping* inputBindings,
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

            DescriptorHeap* newDescriptorHeap(DescriptorHeapType type, uint32_t descriptorCount)override;

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

            [[nodiscard]] Sampler* newSampler(
                SamplerFilter min,
                SamplerFilter mag,
                SamplerFilter mip,
                SamplerAddressMode u,
                SamplerAddressMode v,
                SamplerAddressMode w,
                float mipLODBias,
                bool anisotrophyEnabled,
                uint8_t  maxAnisotrophy,
                ComparisonFunction comparisonFunction,
                Color borderColor,
                float minLOD,
                float maxLOD
                )override;

            //Vulkan specific
            [[nodiscard]] VkDevice device() const;
            [[nodiscard]] VmaAllocator allocator() const;
            [[nodiscard]] uint32_t graphicsFamilyIndex()const;
            [[nodiscard]] uint32_t computeFamilyIndex()const;
            [[nodiscard]] uint32_t transferFamilyIndex()const;

            PFN_vkCmdBindSamplerHeapEXT vkCmdBindSamplerHeap=nullptr;
            PFN_vkCmdBindResourceHeapEXT vkCmdBindResourceHeap=nullptr;
            PFN_vkWriteSamplerDescriptorsEXT vkWriteSamplerDescriptors=nullptr;
            PFN_vkWriteResourceDescriptorsEXT vkWriteResourceDescriptors=nullptr;
            PFN_vkRegisterCustomBorderColorEXT vkRegisterCustomBorderColor=nullptr;
            PFN_vkUnregisterCustomBorderColorEXT vkUnregisterCustomBorderColor=nullptr;
            PFN_vkCmdPushDataEXT vkCmdPushData=nullptr;
            //samplers have a hard limit to how many can be assigned, so let samplers manage that number
            friend class VulkanSampler;
        private:

            void move(VulkanGraphicsCard& from);
            std::string _name;
            GraphicsCardMemoryProperties _memoryProperties{};
            GraphicsCardCapabilities _capabilities{};
            DescriptorHeapDetails _descriptorHeapDetails{};
            VkPhysicalDevice _physicalDevice=nullptr;
            VkDevice _device=nullptr;
            VmaAllocator _allocator = nullptr;
            VulkanSubmissionQueue* _graphicsQueue=nullptr;
            VulkanSubmissionQueue* _computeQueue=nullptr;
            VulkanSubmissionQueue* _transferQueue=nullptr;
            uint32_t _graphicsQueueFamily=0;
            uint32_t _computeQueueFamily=0;
            uint32_t _transferQueueFamily=0;
            uint32_t _allocatedSamplers=0;
        };
    } // vulkan
} // slag

#endif //SLAG_VULKANGRAPHICSCARD_H
