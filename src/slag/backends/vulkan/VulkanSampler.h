#ifndef SLAG_VULKANSAMPLER_H
#define SLAG_VULKANSAMPLER_H
#include "slag/core/Color.h"
#include "slag/core/Sampler.h"
#include <vulkan/vulkan.h>

namespace slag
{
    namespace vulkan
    {
        class VulkanGraphicsCard;

        class VulkanSampler: public Sampler
        {
        public:
            VulkanSampler(
                VulkanGraphicsCard* graphicsCard,
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
                );

            ~VulkanSampler()override;
            VulkanSampler(VulkanSampler&& from) noexcept ;
            VulkanSampler& operator=(VulkanSampler&& from) noexcept;
            VulkanSampler(VulkanSampler&)=delete;
            VulkanSampler& operator=(VulkanSampler&)=delete;
            SamplerFilter magFilter()override;
            SamplerFilter minFilter()override;
            SamplerFilter mipMapFilter()override;
            SamplerAddressMode addressU()override;
            SamplerAddressMode addressV()override;
            SamplerAddressMode addressW()override;
            float mipLODBias()override;
            bool anisotrophyEnabled()override;
            uint8_t maxAnisotrophy()override;
            ComparisonFunction comparisonFunction()override;
            Color borderColor()override;
            float minLOD()override;
            float maxLOD()override;
            GraphicsCard* graphicsCard()override;
            VkSamplerCreateInfo vulkanHandle() const;
        private:
            void move(VulkanSampler& from);
            VkSamplerCreateInfo _createInfo{.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
            VkSamplerCustomBorderColorIndexCreateInfoEXT _borderExt{.sType = VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_INDEX_CREATE_INFO_EXT};
            SamplerFilter _min;
            SamplerFilter _mag;
            SamplerFilter _mip;
            SamplerAddressMode _u;
            SamplerAddressMode _v;
            SamplerAddressMode _w;
            float _mipLODBias=0;
            bool _anisotrophyEnabled=false;
            uint8_t  _maxAnisotrophy=1;
            ComparisonFunction _comparisonFunction;
            Color _borderColor;
            float _minLOD=0;
            float _maxLOD=1000;
            VulkanGraphicsCard* _graphicsCard = nullptr;
        };
    } // vulkan
} // slag

#endif //SLAG_VULKANSAMPLER_H