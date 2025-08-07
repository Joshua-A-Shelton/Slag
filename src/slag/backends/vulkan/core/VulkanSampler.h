#ifndef SLAG_VULKANSAMPLER_H
#define SLAG_VULKANSAMPLER_H
#include <slag/Slag.h>
#include <vulkan/vulkan.h>

namespace slag
{
    namespace vulkan
    {
        class VulkanSampler: public Sampler
        {
            public:
            VulkanSampler(SamplerParameters parameters);
            ~VulkanSampler()override;
            VulkanSampler(VulkanSampler&& from);
            VulkanSampler& operator=(VulkanSampler&& from);
            VulkanSampler(VulkanSampler&)=delete;
            VulkanSampler& operator=(VulkanSampler&)=delete;
            Filter magFilter()override;
            Filter minFilter()override;
            Filter mipMapFilter()override;
            AddressMode addressU()override;
            AddressMode addressV()override;
            AddressMode addressW()override;
            float mipLODBias()override;
            bool anisotrophyEnabled()override;
            uint8_t maxAnisotrophy()override;
            Operations::ComparisonFunction comparisonFunction()override;
            Color borderColor()override;
            float minLOD()override;
            float maxLOD()override;

            VkSampler vulkanHandle();
        private:
            void move(VulkanSampler& from);
            VkSampler _sampler = nullptr;
            Sampler::Filter _min;
            Sampler::Filter _mag;
            Sampler::Filter _mip;
            Sampler::AddressMode _u;
            Sampler::AddressMode _v;
            Sampler::AddressMode _w;
            float _mipLODBias=0;
            bool _anisotrophyEnabled=false;
            uint8_t  _maxAnisotrophy=1;
            Operations::ComparisonFunction _comparisonFunction;
            Color _borderColor;
            float _minLOD=0;
            float _maxLOD=1000;
        };
    } // vulkan
} // slag

#endif //SLAG_VULKANSAMPLER_H
