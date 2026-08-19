#ifndef SLAG_VULKANTEXTURE_H
#define SLAG_VULKANTEXTURE_H
#include <slag/Slag.h>
#include <vulkan/vulkan.h>

#include "vk_mem_alloc.h"

namespace slag
{
    namespace vulkan
    {
        class VulkanGraphicsCard;
        struct VulkanImageMoveData
        {
            bool movedSucceded = false;
            VkImage image=nullptr;
        };

        class VulkanTexture: public Texture
        {
        public:
            VulkanTexture(
                VulkanGraphicsCard* card,
                PixelFormat format,
                TextureUsageFlags usage,
                uint32_t width,
                uint32_t mipLevels,
                uint32_t layers);

            VulkanTexture(
                VulkanGraphicsCard* card,
                PixelFormat format,
                TextureUsageFlags usage,
                uint32_t width,
                uint32_t height,
                uint32_t mipLevels,
                uint32_t layers,
                SampleCount sampleCount);

            VulkanTexture(
                VulkanGraphicsCard* card,
                PixelFormat format,
                TextureUsageFlags usage,
                uint32_t width,
                uint32_t height,
                uint32_t depth,
                uint32_t mipLevels);

            VulkanTexture(
                VulkanGraphicsCard* card,
                uint32_t dimension,
                PixelFormat format,
                TextureUsageFlags usage,
                uint32_t mipLevels,
                uint32_t arrayDepth);

            //create a texture from existing resources, used for swapchain images
            VulkanTexture(VulkanGraphicsCard* card, TextureType type, VkImage image, PixelFormat format, TextureUsageFlags usage, uint32_t width, uint32_t height, uint32_t depth, uint32_t layers, uint32_t mipLevels, SampleCount sampleCount);

            VulkanTexture(const VulkanTexture&)=delete;
            VulkanTexture& operator=(const VulkanTexture&)=delete;
            VulkanTexture(VulkanTexture&& from) noexcept;
            VulkanTexture& operator=(VulkanTexture&& from) noexcept;
            ~VulkanTexture()override;
            [[nodiscard]] uint32_t width()const override;
            [[nodiscard]] uint32_t height()const override;
            [[nodiscard]] uint32_t depth()const override;
            [[nodiscard]] uint32_t layers()const override;
            [[nodiscard]] uint32_t mipLevels()const override;
            [[nodiscard]] PixelFormat format()const override;
            [[nodiscard]] SampleCount sampleCount()const override;
            [[nodiscard]] TextureType type()const override;
            [[nodiscard]] TextureUsageFlags usage()const override;
            [[nodiscard]] GraphicsCard* graphicsCard()override;
            [[nodiscard]] void* userData()override;
            void setUserData(void* userData)override;

            VulkanImageMoveData moveMemory(VmaAllocation tempAllocation,CommandBuffer* transitionToGeneralBuffer, CommandBuffer* copyDataBuffer);

            [[nodiscard]] VkImage vulkanHandle()const;
            [[nodiscard]] const VkImageViewCreateInfo& descriptorInfo();

        private:
            void move(VulkanTexture& from);
            void construct(VkImageType imageType,PixelFormatProperties, VkImageCreateFlags flags);
            void populateDescriptorInfo();
            VkImageViewCreateInfo _descriptorInfo{.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            MemoryReference _selfReference{.type = MemoryObjectType::TEXTURE, .memory = {.texture = this}};
            VulkanGraphicsCard* _graphicsCard = nullptr;
            VmaAllocation _allocation = nullptr;
            VkImage _texture = nullptr;
            void* _userData = nullptr;
            PixelFormat _format = PixelFormat::UNDEFINED;
            TextureUsageFlags _usage = TextureUsageFlags::NONE;
            SampleCount _sampleCount = SampleCount::ONE;
            TextureType _type = TextureType::TWO_DIMENSIONAL;
            uint32_t _width = 1, _height = 1, _depth = 1, _layers = 1, _mipLevels = 1;
        };
    } // vulkan
} // slag

#endif //SLAG_VULKANTEXTURE_H
