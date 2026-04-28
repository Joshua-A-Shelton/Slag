#ifndef SLAG_VULKANBACKEND_H
#define SLAG_VULKANBACKEND_H

#include <vector>

#include "VulkanGraphicsCard.h"
#include "slag/core/IBackend.h"
#include "slag/core/Sampler.h"

namespace slag
{
    namespace vulkan
    {
        struct NativeFormat
        {
            VkFormat format = VK_FORMAT_UNDEFINED;
            VkComponentSwizzle rSwizzle = VK_COMPONENT_SWIZZLE_IDENTITY;
            VkComponentSwizzle gSwizzle = VK_COMPONENT_SWIZZLE_IDENTITY;
            VkComponentSwizzle bSwizzle = VK_COMPONENT_SWIZZLE_IDENTITY;
            VkComponentSwizzle aSwizzle = VK_COMPONENT_SWIZZLE_IDENTITY;
        };
        class VulkanBackend: public IBackend
        {
        public:
            VulkanBackend();
            ~VulkanBackend()override;
            [[nodiscard]] BackendAPI api()const override;
            [[nodiscard]] uint32_t graphicsCardCount()const override;
            [[nodiscard]] GraphicsCard* graphicsCard(uint32_t index)override;
            [[nodiscard]] uint32_t supportedShaderLanguageCount()const override;
            [[nodiscard]] ShaderLanguage supportedShaderLanguage(uint32_t index)const override;

            static VkBufferUsageFlagBits2 nativeBufferUsage(BufferMemoryType access);
            static NativeFormat nativeFormat(PixelFormat format);
            static VkImageUsageFlags nativeTextureUsage(TextureUsageFlags flags);
            inline static VkImageAspectFlags nativeTextureAspect(PixelAspectFlags aspect){return static_cast<VkImageAspectFlagBits>(aspect);}
            static VkAccessFlags2 nativeMemoryCaches(MemoryCaches caches);
            static VkPipelineStageFlags2 nativePipelineStages(SyncStages stages);
            static VkPolygonMode nativePolygonMode(RasterizationState::DrawMode mode);
            static VkCullModeFlags nativeCullMode(RasterizationState::CullOptions mode);
            static VkFrontFace nativeFrontFace(RasterizationState::FrontFacing frontFace);
            static VkBlendFactor nativeBlendFactor(BlendFactor blendFactor);
            static VkBlendOp nativeBlendOp(BlendOperation blendOp);
            static VkLogicOp nativeLogicOp(LogicOperation logicOp);
            static VkStencilOp nativeStencilOp(StencilOperation stencilOp);
            static VkColorComponentFlags nativeColorComponentFlags(ColorComponents colorComponentFlags);
            static VkCompareOp nativeCompareOp(ComparisonFunction compareOp);
            static VkFilter nativeFilter(SamplerFilter filter);
            static VkSamplerAddressMode nativeSamplerAddressMode(SamplerAddressMode mode);
        private:
            SlagInitializationResult initializeBackend(const InitializationData& initializationData)override;
            std::vector<VulkanGraphicsCard> _graphicsCards;
            vkb::Instance _instance{};
            VkDebugUtilsMessengerEXT _debugMessenger=nullptr;
        };
    } // vulkan
} // slag

#endif //SLAG_VULKANBACKEND_H
