#ifndef SLAG_VULKANTEXTURE_H
#define SLAG_VULKANTEXTURE_H
#include <slag/Slag.h>
#include <vulkan/vulkan.h>
#include "VulkanGPUMemoryReference.h"
#include "vk_mem_alloc.h"
#include "slag/core/Pixels.h"

namespace slag
{
    namespace vulkan
    {
        struct VulkanImageMoveData
        {
            bool movedSucceded = false;
            VkImage image=nullptr;
            VkImageView view=nullptr;
        };

        class VulkanTexture: public Texture
        {
        public:
            VulkanTexture(Pixels::Format texelFormat, Type type, UsageFlags usageFlags, uint32_t width, uint32_t depth, uint32_t height,uint32_t mipLevels, uint32_t layers, Texture::SampleCount sampleCount);
            VulkanTexture(Pixels::Format texelFormat, Type type, UsageFlags usageFlags, uint32_t width, uint32_t depth, uint32_t height,uint32_t mipLevels, uint32_t layers, Texture::SampleCount sampleCount, void* texelData, uint64_t texelDataLength, TextureBufferMapping* mappings, uint32_t mappingCount);
            /**
             * Create a texture from existing elements, memory is not owned, and must be managed by caller
             * @param image
             * @param format
             * @param type
             * @param width
             * @param height
             * @param layers
             * @param mipLevels
             * @param usageFlags
             * @param sampleCount
             */
            VulkanTexture(VkImage image, VkImageView view, Pixels::Format format, Type type, uint32_t width, uint32_t height, uint32_t depth,uint32_t mipLevels, uint32_t layers, UsageFlags usageFlags, Texture::SampleCount sampleCount);
            ~VulkanTexture()override;
            VulkanTexture(const VulkanTexture&) = delete;
            VulkanTexture& operator=(const VulkanTexture&) = delete;
            VulkanTexture(VulkanTexture&& from);
            VulkanTexture& operator=(VulkanTexture&& from);
            ///What kind of type this texture is
            virtual Type type()override;
            ///What kind of usage does this texture support
            virtual UsageFlags usageFlags()override;
            ///Samples used in multisampling
            virtual SampleCount sampleCount()override;
            ///Width in texels
            virtual uint32_t width()override;
            ///Height in pixels
            virtual uint32_t height()override;
            ///Number of depth slices in 3D textures, 1 in everything else
            virtual uint32_t depth()override;
            ///Number of elements in the array (1D or 2D textures), or number of depth slices (3D texture), (or 6 in cubemaps, one for each face of the cube)
            virtual uint32_t layers()override;
            ///Number of mip levels (lower LOD images used in shader sampling)
            virtual uint32_t mipLevels()override;
            ///The type of texel format backing the image
            virtual Pixels::Format format()override;

            VkImage vulkanHandle();
            VkImageView vulkanViewHandle();

            VulkanImageMoveData moveMemory(VmaAllocation tempAllocation,CommandBuffer* transitionToGeneralBuffer, CommandBuffer* copyDataBuffer);

            static void initializeChromaConverters();
            static void cleanupChromaConverters();


        private:
            Pixels::Format _format = Pixels::Format::UNDEFINED;
            Type _type = Type::TEXTURE_2D;
            UsageFlags _usageFlags = UsageFlags::SAMPLED_IMAGE;
            SampleCount _sampleCount = SampleCount::ONE;
            uint32_t _width=0;
            uint32_t _height=0;
            uint32_t _depth=0;
            uint32_t _layers=1;
            uint32_t _mipLevels=1;
            VkImage _image=VK_NULL_HANDLE;
            VkImageView _view=VK_NULL_HANDLE;
            VmaAllocation _allocation=VK_NULL_HANDLE;
            VulkanGPUMemoryReference _selfReference{.memoryType = VulkanGPUMemoryReference::MemoryType::TEXTURE, .reference = {this}};
            void move(VulkanTexture& texture);
            void initialize(Pixels::Format texelFormat, Type type, UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, uint32_t layers, Texture::SampleCount sampleCount, VkImageLayout initialLayout);

            static inline VkSamplerYcbcrConversion NV12_CONVERTER=nullptr;
            static inline VkSamplerYcbcrConversion OPAQUE_420_CONVERTER=nullptr;

        };
    } // vulkan
} // slag

#endif //SLAG_VULKANTEXTURE_H
